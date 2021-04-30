// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGeoreference.h"
#include "Camera/PlayerCameraManager.h"
#include "CesiumGeoreferenceListener.h"
#include "CesiumBoundingVolumeProvider.h"
#include "CesiumGeospatial/Transforms.h"
#include "CesiumRuntime.h"
#include "CesiumTransforms.h"
#include "CesiumUtility/Math.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Math/Matrix.h"
#include "Math/RotationTranslationMatrix.h"
#include "Math/Rotator.h"
#include "Math/Vector.h"
#include "Misc/PackageName.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <optional>

#if WITH_EDITOR
#include "DrawDebugHelpers.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "Slate/SceneViewport.h"
#endif

/*static*/ ACesiumGeoreference*
ACesiumGeoreference::GetDefaultForActor(AActor* Actor) {
  ACesiumGeoreference* pGeoreference = FindObject<ACesiumGeoreference>(
      Actor->GetLevel(),
      TEXT("CesiumGeoreferenceDefault"));
  if (!pGeoreference) {
    const FString actorName = Actor->GetName();
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("Creating default Georeference for actor %s"),
        *actorName);
    FActorSpawnParameters spawnParameters;
    spawnParameters.Name = TEXT("CesiumGeoreferenceDefault");
    spawnParameters.OverrideLevel = Actor->GetLevel();
    pGeoreference =
        Actor->GetWorld()->SpawnActor<ACesiumGeoreference>(spawnParameters);
  } else {
    const FString georeferenceName = pGeoreference->GetName();
    const FString actorName = Actor->GetName();
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("Using existing Georeference %s for actor %s"),
        *georeferenceName,
        *actorName);
  }
  return pGeoreference;
}

ACesiumGeoreference::ACesiumGeoreference() : _insideSublevel(false) {
  PrimaryActorTick.bCanEverTick = true;
}

void ACesiumGeoreference::PlaceGeoreferenceOriginHere() {
#if WITH_EDITOR
  // TODO: should we just assume origin rebasing isn't happening since this is
  // only editor-mode?

  // If this is PIE mode, ignore
  if (this->GetWorld()->IsGameWorld()) {
    return;
  }

  FViewport* pViewport = GEditor->GetActiveViewport();
  FViewportClient* pViewportClient = pViewport->GetClient();
  FEditorViewportClient* pEditorViewportClient =
      static_cast<FEditorViewportClient*>(pViewportClient);

  FRotationTranslationMatrix fCameraTransform(
      pEditorViewportClient->GetViewRotation(),
      pEditorViewportClient->GetViewLocation());
  const FIntVector& originLocation = this->GetWorld()->OriginLocation;

  // TODO: optimize this, only need to transform the front direction and
  // translation

  // camera local space to Unreal absolute world
  glm::dmat4 cameraToAbsolute(
      glm::dvec4(
          fCameraTransform.M[0][0],
          fCameraTransform.M[0][1],
          fCameraTransform.M[0][2],
          0.0),
      glm::dvec4(
          fCameraTransform.M[1][0],
          fCameraTransform.M[1][1],
          fCameraTransform.M[1][2],
          0.0),
      glm::dvec4(
          fCameraTransform.M[2][0],
          fCameraTransform.M[2][1],
          fCameraTransform.M[2][2],
          0.0),
      glm::dvec4(
          static_cast<double>(fCameraTransform.M[3][0]) +
              static_cast<double>(originLocation.X),
          static_cast<double>(fCameraTransform.M[3][1]) +
              static_cast<double>(originLocation.Y),
          static_cast<double>(fCameraTransform.M[3][2]) +
              static_cast<double>(originLocation.Z),
          1.0));

  // camera local space to ECEF
  glm::dmat4 cameraToECEF = this->_ueAbsToEcef * cameraToAbsolute;

  // Long/Lat/Height camera location (also our new target georeference origin)
  std::optional<CesiumGeospatial::Cartographic> targetGeoreferenceOrigin =
      CesiumGeospatial::Ellipsoid::WGS84.cartesianToCartographic(
          cameraToECEF[3]);

  if (!targetGeoreferenceOrigin) {
    // This only happens when the location is too close to the center of the
    // Earth.
    return;
  }

  this->_setGeoreferenceOrigin(
      glm::degrees(targetGeoreferenceOrigin->longitude),
      glm::degrees(targetGeoreferenceOrigin->latitude),
      targetGeoreferenceOrigin->height);

  glm::dmat4 absoluteToRelativeWorld(
      glm::dvec4(1.0, 0.0, 0.0, 0.0),
      glm::dvec4(0.0, 1.0, 0.0, 0.0),
      glm::dvec4(0.0, 0.0, 1.0, 0.0),
      glm::dvec4(-originLocation.X, -originLocation.Y, -originLocation.Z, 1.0));

  // TODO: check for degeneracy ?
  glm::dmat4 newCameraTransform =
      absoluteToRelativeWorld * this->_ecefToUeAbs * cameraToECEF;
  glm::dvec3 cameraFront = glm::normalize(newCameraTransform[0]);
  glm::dvec3 cameraRight =
      glm::normalize(glm::cross(glm::dvec3(0.0, 0.0, 1.0), cameraFront));
  glm::dvec3 cameraUp = glm::normalize(glm::cross(cameraFront, cameraRight));

  pEditorViewportClient->SetViewRotation(
      FMatrix(
          FVector(cameraFront.x, cameraFront.y, cameraFront.z),
          FVector(cameraRight.x, cameraRight.y, cameraRight.z),
          FVector(cameraUp.x, cameraUp.y, cameraUp.z),
          FVector(0.0f, 0.0f, 0.0f))
          .Rotator());
  pEditorViewportClient->SetViewLocation(
      FVector(-originLocation.X, -originLocation.Y, -originLocation.Z));
#endif
}

void ACesiumGeoreference::CheckForNewSubLevels() {
  const TArray<ULevelStreaming*>& streamedLevels =
      this->GetWorld()->GetStreamingLevels();
  // check all levels to see if any are new
  for (ULevelStreaming* streamedLevel : streamedLevels) {
    FString levelName =
        FPackageName::GetShortName(streamedLevel->GetWorldAssetPackageName());
    levelName.RemoveFromStart(this->GetWorld()->StreamingLevelsPrefix);
    // check the known levels to see if this one is new
    bool found = false;
    for (FCesiumSubLevel& subLevel : this->CesiumSubLevels) {
      if (levelName.Equals(subLevel.LevelName)) {
        found = true;
        break;
      }
    }

    if (!found) {
      // add this level to the known streaming levels
      this->CesiumSubLevels.Add(FCesiumSubLevel{
          levelName,
          OriginLongitude,
          OriginLatitude,
          OriginHeight,
          1000.0, // TODO: figure out better default radius
          false});
    }
  }
}

void ACesiumGeoreference::JumpToCurrentLevel() {
  if (this->CurrentLevelIndex < 0 ||
      this->CurrentLevelIndex >= this->CesiumSubLevels.Num()) {
    return;
  }

  const FCesiumSubLevel& currentLevel =
      this->CesiumSubLevels[this->CurrentLevelIndex];

  this->SetGeoreferenceOrigin(glm::dvec3(
      currentLevel.LevelLongitude,
      currentLevel.LevelLatitude,
      currentLevel.LevelHeight));
}

void ACesiumGeoreference::SetGeoreferenceOrigin(
    const glm::dvec3& targetLongitudeLatitudeHeight) {
  // Should not allow externally initiated georeference origin changing if we
  // are inside a sublevel
  if (this->_insideSublevel) {
    return;
  }
  this->_setGeoreferenceOrigin(
      targetLongitudeLatitudeHeight.x,
      targetLongitudeLatitudeHeight.y,
      targetLongitudeLatitudeHeight.z);
}

void ACesiumGeoreference::InaccurateSetGeoreferenceOrigin(
    const FVector& targetLongitudeLatitudeHeight) {
  this->SetGeoreferenceOrigin(glm::dvec3(
      targetLongitudeLatitudeHeight.X,
      targetLongitudeLatitudeHeight.Y,
      targetLongitudeLatitudeHeight.Z));
}

void ACesiumGeoreference::AddGeoreferenceListener(
    ICesiumGeoreferenceListener* Object) {

  // avoid adding duplicates
  for (TWeakInterfacePtr<ICesiumGeoreferenceListener> pObject :
       this->_georeferenceListeners) {
    if (Cast<ICesiumGeoreferenceListener>(pObject.GetObject()) == Object) {
      return;
    }
  }

  this->_georeferenceListeners.Add(*Object);

  // If this object is an Actor or UActorComponent, make sure it ticks _after_
  // the CesiumGeoreference.
  AActor* pActor = Cast<AActor>(Object);
  UActorComponent* pActorComponent = Cast<UActorComponent>(Object);
  if (pActor) {
    pActor->AddTickPrerequisiteActor(this);
  } else if (pActorComponent) {
    pActorComponent->AddTickPrerequisiteActor(this);
  }

  // TODO: try commenting this out, it shouldn't be needed
  this->UpdateGeoreference();
}

void ACesiumGeoreference::AddBoundingVolumeProvider(
    ICesiumBoundingVolumeProvider* Object) {
  
  this->_boundingVolumeProviders.Add(*Object);

  if (this->OriginPlacement == EOriginPlacement::BoundingVolumeOrigin) {
    this->UpdateGeoreference();
  }
}

// Called when the game starts or when spawned
void ACesiumGeoreference::BeginPlay() {
  Super::BeginPlay();

  if (!this->WorldOriginCamera) {
    // Find the first player's camera manager
    APlayerController* pPlayerController =
        this->GetWorld()->GetFirstPlayerController();
    if (pPlayerController) {
      this->WorldOriginCamera = pPlayerController->PlayerCameraManager;
    }
  }

  // initialize sublevels as unloaded
  for (FCesiumSubLevel& level : CesiumSubLevels) {
    level.CurrentlyLoaded = false;
  }
}

/** In case the CesiumGeoreference gets spawned at run time, instead of design
 *  time, ensure that frames are updated */
void ACesiumGeoreference::OnConstruction(const FTransform& Transform) {
  this->UpdateGeoreference();
}

void ACesiumGeoreference::UpdateGeoreference() {
  // update georeferenced -> ECEF
  if (this->OriginPlacement == EOriginPlacement::TrueOrigin) {
    this->_georeferencedToEcef = glm::dmat4(1.0);
  } else {
    glm::dvec3 center(0.0, 0.0, 0.0);

    if (this->OriginPlacement == EOriginPlacement::BoundingVolumeOrigin) {
      // TODO: it'd be better to compute the union of the bounding volumes and
      // then use the union's center,
      //       rather than averaging the centers.
      size_t numberOfPositions = 0;

      for (const TWeakInterfacePtr<ICesiumBoundingVolumeProvider>& pObject :
           this->_boundingVolumeProviders) {
        if (pObject.IsValid()) {
          std::optional<Cesium3DTiles::BoundingVolume> bv =
              pObject->GetBoundingVolume();
          if (bv) {
            center += Cesium3DTiles::getBoundingVolumeCenter(*bv);
            ++numberOfPositions;
          }
        }
      }

      if (numberOfPositions > 0) {
        center /= numberOfPositions;
      }
    } else if (this->OriginPlacement == EOriginPlacement::CartographicOrigin) {
      const CesiumGeospatial::Ellipsoid& ellipsoid =
          CesiumGeospatial::Ellipsoid::WGS84;
      center = ellipsoid.cartographicToCartesian(
          CesiumGeospatial::Cartographic::fromDegrees(
              this->OriginLongitude,
              this->OriginLatitude,
              this->OriginHeight));
    }

    this->_georeferencedToEcef =
        CesiumGeospatial::Transforms::eastNorthUpToFixedFrame(center);
  }

  // update ECEF -> georeferenced
  this->_ecefToGeoreferenced = glm::affineInverse(this->_georeferencedToEcef);

  // update UE -> ECEF
  this->_ueAbsToEcef = this->_georeferencedToEcef *
                       CesiumTransforms::scaleToCesium *
                       CesiumTransforms::unrealToOrFromCesium;

  // update ECEF -> UE
  this->_ecefToUeAbs = CesiumTransforms::unrealToOrFromCesium *
                       CesiumTransforms::scaleToUnrealWorld *
                       this->_ecefToGeoreferenced;

  for (TWeakInterfacePtr<ICesiumGeoreferenceListener> pObject :
       this->_georeferenceListeners) {
    if (pObject.IsValid()) {
      pObject->NotifyGeoreferenceUpdated();
    }
  }

  this->_setSunSky(this->OriginLongitude, this->OriginLatitude);
}

#if WITH_EDITOR
void ACesiumGeoreference::PostEditChangeProperty(FPropertyChangedEvent& event) {
  Super::PostEditChangeProperty(event);

  if (!event.Property) {
    return;
  }

  FName propertyName = event.Property->GetFName();

  if (propertyName ==
          GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, OriginPlacement) ||
      propertyName ==
          GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, OriginLongitude) ||
      propertyName ==
          GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, OriginLatitude) ||
      propertyName ==
          GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, OriginHeight) ||
      propertyName == GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, SunSky)) {
    this->UpdateGeoreference();
    return;
  } else if (
      propertyName ==
      GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, CurrentLevelIndex)) {
    this->JumpToCurrentLevel();
  } else if (
      propertyName ==
      GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, CesiumSubLevels)) {
  }
}
#endif

// Called every frame
bool ACesiumGeoreference::ShouldTickIfViewportsOnly() const { return true; }

void ACesiumGeoreference::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);

  bool isGame = this->GetWorld()->IsGameWorld();
  const FIntVector& originLocation = this->GetWorld()->OriginLocation;

#if WITH_EDITOR
  if (!isGame) {
    if (this->ShowLoadRadii) {
      for (FCesiumSubLevel& level : this->CesiumSubLevels) {
        glm::dvec3 levelECEF =
            CesiumGeospatial::Ellipsoid::WGS84.cartographicToCartesian(
                CesiumGeospatial::Cartographic::fromDegrees(
                    level.LevelLongitude,
                    level.LevelLatitude,
                    level.LevelHeight));

        glm::dvec4 levelAbs = this->_ecefToUeAbs * glm::dvec4(levelECEF, 1.0);
        FVector levelRelative = FVector(levelAbs.x, levelAbs.y, levelAbs.z) -
                                FVector(originLocation);
        DrawDebugSphere(
            this->GetWorld(),
            levelRelative,
            100.0 * level.LoadRadius,
            100,
            FColor::Blue);
      }
    }

    if (this->EditOriginInViewport) {
      FHitResult mouseRayResults;
      bool mouseRaySuccess;

      this->_lineTraceViewportMouse(false, mouseRaySuccess, mouseRayResults);

      if (mouseRaySuccess) {
        FVector grabbedLocation = mouseRayResults.Location;
        // convert from UE to ECEF to LongitudeLatitudeHeight
        glm::dvec4 grabbedLocationAbs(
            static_cast<double>(grabbedLocation.X) +
                static_cast<double>(originLocation.X),
            static_cast<double>(grabbedLocation.Y) +
                static_cast<double>(originLocation.Y),
            static_cast<double>(grabbedLocation.Z) +
                static_cast<double>(originLocation.Z),
            1.0);

        glm::dvec3 grabbedLocationECEF =
            this->_ueAbsToEcef * grabbedLocationAbs;
        std::optional<CesiumGeospatial::Cartographic> optCartographic =
            CesiumGeospatial::Ellipsoid::WGS84.cartesianToCartographic(
                grabbedLocationECEF);

        if (optCartographic) {
          CesiumGeospatial::Cartographic cartographic = *optCartographic;
          UE_LOG(
              LogActor,
              Warning,
              TEXT("Mouse Location: (Longitude: %f, Latitude: %f, Height: %f)"),
              glm::degrees(cartographic.longitude),
              glm::degrees(cartographic.latitude),
              cartographic.height);

          // TODO: find editor viewport mouse click event
          // if (mouseDown) {
          // this->_setGeoreferenceOrigin()
          //	this->EditOriginInViewport = false;
          //}
        }
      }
    }
  }
#endif

  if (isGame && this->WorldOriginCamera) {
    const FMinimalViewInfo& pov = this->WorldOriginCamera->ViewTarget.POV;
    const FVector& cameraLocation = pov.Location;

    glm::dvec4 cameraAbsolute(
        static_cast<double>(cameraLocation.X) +
            static_cast<double>(originLocation.X),
        static_cast<double>(cameraLocation.Y) +
            static_cast<double>(originLocation.Y),
        static_cast<double>(cameraLocation.Z) +
            static_cast<double>(originLocation.Z),
        1.0);

    glm::dvec3 cameraECEF = this->_ueAbsToEcef * cameraAbsolute;

    this->_insideSublevel = false;

    const TArray<ULevelStreaming*>& streamedLevels =
        this->GetWorld()->GetStreamingLevels();
    for (ULevelStreaming* streamedLevel : streamedLevels) {
      FString levelName =
          FPackageName::GetShortName(streamedLevel->GetWorldAssetPackageName());
      levelName.RemoveFromStart(this->GetWorld()->StreamingLevelsPrefix);
      // TODO: maybe we should precalculate the level ECEF from level
      // long/lat/height
      // TODO: consider the case where we're intersecting multiple level radii
      for (FCesiumSubLevel& level : this->CesiumSubLevels) {
        // if this is a known level, we need to tell it whether or not it should
        // be loaded
        if (levelName.Equals(level.LevelName)) {
          glm::dvec3 levelECEF =
              CesiumGeospatial::Ellipsoid::WGS84.cartographicToCartesian(
                  CesiumGeospatial::Cartographic::fromDegrees(
                      level.LevelLongitude,
                      level.LevelLatitude,
                      level.LevelHeight));

          if (glm::length(levelECEF - cameraECEF) < level.LoadRadius) {
            this->_insideSublevel = true;
            if (!level.CurrentlyLoaded) {
              this->_jumpToLevel(level);
              streamedLevel->SetShouldBeLoaded(true);
              streamedLevel->SetShouldBeVisible(true);
              level.CurrentlyLoaded = true;

              // If we are not going to continue origin rebasing inside the
              // sublevel, just set the origin back to zero since the sublevel
              // will be centered around zero anyways.
              if (this->KeepWorldOriginNearCamera &&
                  !this->OriginRebaseInsideSublevels) {
                this->GetWorld()->SetNewWorldOrigin(FIntVector(0, 0, 0));
              }
            }
          } else {
            if (level.CurrentlyLoaded) {
              streamedLevel->SetShouldBeLoaded(false);
              streamedLevel->SetShouldBeVisible(false);
              level.CurrentlyLoaded = false;
            }
          }

          break;
        }
      }
    }

    /*
    // EXPERIMENTAL GEOREFERENCE REBASING
    if (!this->_insideSublevel && glm::length(cameraECEF -
    glm::dvec3(this->_ueAbsToEcef[3])) > 1000000.0) {
            std::optional<CesiumGeospatial::Cartographic>
    targetGeoreferenceOrigin =
    CesiumGeospatial::Ellipsoid::WGS84.cartesianToCartographic(cameraECEF); if
    (targetGeoreferenceOrigin) {
                    this->_setGeoreferenceOrigin(glm::degrees(targetGeoreferenceOrigin->longitude),
    glm::degrees(targetGeoreferenceOrigin->latitude),
    targetGeoreferenceOrigin->height);
            }
    }
    */

    if (this->KeepWorldOriginNearCamera &&
        (!this->_insideSublevel || this->OriginRebaseInsideSublevels) &&
        !cameraLocation.Equals(
            FVector(0.0f, 0.0f, 0.0f),
            this->MaximumWorldOriginDistanceFromCamera)) {
      // Camera has moved too far from the origin, move the origin.
      this->GetWorld()->SetNewWorldOrigin(FIntVector(
          static_cast<int32>(cameraLocation.X) +
              static_cast<int32>(originLocation.X),
          static_cast<int32>(cameraLocation.Y) +
              static_cast<int32>(originLocation.Y),
          static_cast<int32>(cameraLocation.Z) +
              static_cast<int32>(originLocation.Z)));
    }
  }
}

/**
 * Useful Conversion Functions
 */

glm::dvec3 ACesiumGeoreference::TransformLongitudeLatitudeHeightToEcef(
    const glm::dvec3& longitudeLatitudeHeight) const {
  return CesiumGeospatial::Ellipsoid::WGS84.cartographicToCartesian(
      CesiumGeospatial::Cartographic::fromDegrees(
          longitudeLatitudeHeight.x,
          longitudeLatitudeHeight.y,
          longitudeLatitudeHeight.z));
}

FVector ACesiumGeoreference::InaccurateTransformLongitudeLatitudeHeightToEcef(
    const FVector& longitudeLatitudeHeight) const {
  glm::dvec3 ecef = this->TransformLongitudeLatitudeHeightToEcef(glm::dvec3(
      longitudeLatitudeHeight.X,
      longitudeLatitudeHeight.Y,
      longitudeLatitudeHeight.Z));
  return FVector(ecef.x, ecef.y, ecef.z);
}

glm::dvec3 ACesiumGeoreference::TransformEcefToLongitudeLatitudeHeight(
    const glm::dvec3& ecef) const {
  std::optional<CesiumGeospatial::Cartographic> llh =
      CesiumGeospatial::Ellipsoid::WGS84.cartesianToCartographic(ecef);
  if (!llh) {
    // TODO: since degenerate cases only happen close to Earth's center
    // would it make more sense to assign an arbitrary but correct LLH
    // coordinate to this case such as (0.0, 0.0, -_EARTH_RADIUS_)?
    return glm::dvec3(0.0, 0.0, 0.0);
  }
  return glm::dvec3(
      glm::degrees(llh->longitude),
      glm::degrees(llh->latitude),
      llh->height);
}

FVector ACesiumGeoreference::InaccurateTransformEcefToLongitudeLatitudeHeight(
    const FVector& ecef) const {
  glm::dvec3 llh = this->TransformEcefToLongitudeLatitudeHeight(
      glm::dvec3(ecef.X, ecef.Y, ecef.Z));
  return FVector(llh.x, llh.y, llh.z);
}

glm::dvec3 ACesiumGeoreference::TransformLongitudeLatitudeHeightToUe(
    const glm::dvec3& longitudeLatitudeHeight) const {
  glm::dvec3 ecef =
      this->TransformLongitudeLatitudeHeightToEcef(longitudeLatitudeHeight);
  return this->TransformEcefToUe(ecef);
}

FVector ACesiumGeoreference::InaccurateTransformLongitudeLatitudeHeightToUe(
    const FVector& longitudeLatitudeHeight) const {
  glm::dvec3 ue = this->TransformLongitudeLatitudeHeightToUe(glm::dvec3(
      longitudeLatitudeHeight.X,
      longitudeLatitudeHeight.Y,
      longitudeLatitudeHeight.Z));
  return FVector(ue.x, ue.y, ue.z);
}

glm::dvec3 ACesiumGeoreference::TransformUeToLongitudeLatitudeHeight(
    const glm::dvec3& ue) const {
  glm::dvec3 ecef = this->TransformUeToEcef(ue);
  return this->TransformEcefToLongitudeLatitudeHeight(ecef);
}

FVector ACesiumGeoreference::InaccurateTransformUeToLongitudeLatitudeHeight(
    const FVector& ue) const {
  glm::dvec3 llh =
      this->TransformUeToLongitudeLatitudeHeight(glm::dvec3(ue.X, ue.Y, ue.Z));
  return FVector(llh.x, llh.y, llh.z);
}

glm::dvec3
ACesiumGeoreference::TransformEcefToUe(const glm::dvec3& ecef) const {
  glm::dvec3 ueAbs = this->_ecefToUeAbs * glm::dvec4(ecef, 1.0);

  const FIntVector& originLocation = this->GetWorld()->OriginLocation;
  return ueAbs -
         glm::dvec3(originLocation.X, originLocation.Y, originLocation.Z);
}

FVector
ACesiumGeoreference::InaccurateTransformEcefToUe(const FVector& ecef) const {
  glm::dvec3 ue = this->TransformEcefToUe(glm::dvec3(ecef.X, ecef.Y, ecef.Z));
  return FVector(ue.x, ue.y, ue.z);
}

glm::dvec3 ACesiumGeoreference::TransformUeToEcef(const glm::dvec3& ue) const {

  if (!IsValid(this->GetWorld())) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("The CesiumGeoreference is not spawned in a level"));
    return ue;
  }
  const FIntVector& originLocation = this->GetWorld()->OriginLocation;
  glm::dvec4 ueAbs(
      ue.x + static_cast<double>(originLocation.X),
      ue.y + static_cast<double>(originLocation.Y),
      ue.z + static_cast<double>(originLocation.Z),
      1.0);
  return this->_ueAbsToEcef * ueAbs;
}

FVector
ACesiumGeoreference::InaccurateTransformUeToEcef(const FVector& ue) const {
  glm::dvec3 ecef = this->TransformUeToEcef(glm::dvec3(ue.X, ue.Y, ue.Z));
  return FVector(ecef.x, ecef.y, ecef.z);
}

FRotator ACesiumGeoreference::TransformRotatorUeToEnu(
    const FRotator& UERotator,
    const glm::dvec3& ueLocation) const {
  glm::dmat3 enuToFixedUE = this->ComputeEastNorthUpToUnreal(ueLocation);

  FMatrix enuAdjustmentMatrix(
      FVector(enuToFixedUE[0].x, enuToFixedUE[0].y, enuToFixedUE[0].z),
      FVector(enuToFixedUE[1].x, enuToFixedUE[1].y, enuToFixedUE[1].z),
      FVector(enuToFixedUE[2].x, enuToFixedUE[2].y, enuToFixedUE[2].z),
      FVector(0.0, 0.0, 0.0));

  return FRotator(enuAdjustmentMatrix.ToQuat() * UERotator.Quaternion());
}

FRotator ACesiumGeoreference::InaccurateTransformRotatorUeToEnu(
    const FRotator& UERotator,
    const FVector& ueLocation) const {
  return this->TransformRotatorUeToEnu(
      UERotator,
      glm::dvec3(ueLocation.X, ueLocation.Y, ueLocation.Z));
}

FRotator ACesiumGeoreference::TransformRotatorEnuToUe(
    const FRotator& ENURotator,
    const glm::dvec3& ueLocation) const {
  glm::dmat3 enuToFixedUE = this->ComputeEastNorthUpToUnreal(ueLocation);
  FMatrix enuAdjustmentMatrix(
      FVector(enuToFixedUE[0].x, enuToFixedUE[0].y, enuToFixedUE[0].z),
      FVector(enuToFixedUE[1].x, enuToFixedUE[1].y, enuToFixedUE[1].z),
      FVector(enuToFixedUE[2].x, enuToFixedUE[2].y, enuToFixedUE[2].z),
      FVector(0.0, 0.0, 0.0));

  FMatrix inverse = enuAdjustmentMatrix.InverseFast();

  return FRotator(inverse.ToQuat() * ENURotator.Quaternion());
}

FRotator ACesiumGeoreference::InaccurateTransformRotatorEnuToUe(
    const FRotator& ENURotator,
    const FVector& ueLocation) const {
  return this->TransformRotatorEnuToUe(
      ENURotator,
      glm::dvec3(ueLocation.X, ueLocation.Y, ueLocation.Z));
}

glm::dmat3
ACesiumGeoreference::ComputeEastNorthUpToUnreal(const glm::dvec3& ue) const {
  glm::dvec3 ecef = this->TransformUeToEcef(ue);
  glm::dmat3 enuToEcef = this->ComputeEastNorthUpToEcef(ecef);

  // Camera Axes = ENU
  // Unreal Axes = controlled by Georeference
  glm::dmat3 rotationCesium =
      glm::dmat3(this->_ecefToGeoreferenced) * enuToEcef;

  return glm::dmat3(CesiumTransforms::unrealToOrFromCesium) * rotationCesium *
         glm::dmat3(CesiumTransforms::unrealToOrFromCesium);
}

FMatrix ACesiumGeoreference::InaccurateComputeEastNorthUpToUnreal(
    const FVector& ue) const {
  glm::dmat3 enuToUnreal =
      this->ComputeEastNorthUpToUnreal(glm::dvec3(ue.X, ue.Y, ue.Z));

  return FMatrix(
      FVector(enuToUnreal[0].x, enuToUnreal[0].y, enuToUnreal[0].z),
      FVector(enuToUnreal[1].x, enuToUnreal[1].y, enuToUnreal[1].z),
      FVector(enuToUnreal[2].x, enuToUnreal[2].y, enuToUnreal[2].z),
      FVector(0.0, 0.0, 0.0));
}

glm::dmat3
ACesiumGeoreference::ComputeEastNorthUpToEcef(const glm::dvec3& ecef) const {
  return glm::dmat3(
      CesiumGeospatial::Transforms::eastNorthUpToFixedFrame(ecef));
}

FMatrix ACesiumGeoreference::InaccurateComputeEastNorthUpToEcef(
    const FVector& ecef) const {
  glm::dmat3 enuToEcef =
      this->ComputeEastNorthUpToEcef(glm::dvec3(ecef.X, ecef.Y, ecef.Z));

  return FMatrix(
      FVector(enuToEcef[0].x, enuToEcef[0].y, enuToEcef[0].z),
      FVector(enuToEcef[1].x, enuToEcef[1].y, enuToEcef[1].z),
      FVector(enuToEcef[2].x, enuToEcef[2].y, enuToEcef[2].z),
      FVector(0.0, 0.0, 0.0));
}

/**
 * Private Helper Functions
 */

void ACesiumGeoreference::_setGeoreferenceOrigin(
    double targetLongitude,
    double targetLatitude,
    double targetHeight) {
  this->OriginLongitude = targetLongitude;
  this->OriginLatitude = targetLatitude;
  this->OriginHeight = targetHeight;

  this->UpdateGeoreference();
}

void ACesiumGeoreference::_jumpToLevel(const FCesiumSubLevel& level) {
  this->_setGeoreferenceOrigin(
      level.LevelLongitude,
      level.LevelLatitude,
      level.LevelHeight);
}

// TODO: Figure out if sunsky can ever be oriented so it's not at the top of the
// planet. Without this sunsky will only look good when we set the georeference
// origin to be near the camera.
void ACesiumGeoreference::_setSunSky(double longitude, double latitude) {
  if (!this->SunSky) {
    return;
  }

  // SunSky needs to be clamped to the ellipsoid surface at this long/lat
  glm::dvec3 targetEcef =
      CesiumGeospatial::Ellipsoid::WGS84.cartographicToCartesian(
          CesiumGeospatial::Cartographic::fromDegrees(
              longitude,
              latitude,
              0.0));
  glm::dvec4 targetAbsUe = this->_ecefToUeAbs * glm::dvec4(targetEcef, 1.0);

  const FIntVector& originLocation = this->GetWorld()->OriginLocation;
  this->SunSky->SetActorLocation(
      FVector(targetAbsUe.x, targetAbsUe.y, targetAbsUe.z) -
      FVector(originLocation));

  UClass* SunSkyClass = this->SunSky->GetClass();
  static FName LongProp = TEXT("Longitude");
  static FName LatProp = TEXT("Latitude");
  for (TFieldIterator<FProperty> PropertyIterator(SunSkyClass);
       PropertyIterator;
       ++PropertyIterator) {
    FProperty* Property = *PropertyIterator;
    FName const PropertyName = Property->GetFName();
    if (PropertyName == LongProp) {
      FFloatProperty* floatProp = CastField<FFloatProperty>(Property);
      if (floatProp) {
        floatProp->SetPropertyValue_InContainer((void*)this->SunSky, longitude);
      }
    } else if (PropertyName == LatProp) {
      FFloatProperty* floatProp = CastField<FFloatProperty>(Property);
      if (floatProp) {
        floatProp->SetPropertyValue_InContainer((void*)this->SunSky, latitude);
      }
    }
  }
  UFunction* UpdateSun = this->SunSky->FindFunction(TEXT("UpdateSun"));
  if (UpdateSun) {
    this->SunSky->ProcessEvent(UpdateSun, NULL);
  }
}

// TODO: should consider raycasting the WGS84 ellipsoid instead. The Unreal
// raycast seems to be inaccurate at glancing angles, perhaps due to the large
// single-precision distances.
#if WITH_EDITOR
void ACesiumGeoreference::_lineTraceViewportMouse(
    const bool ShowTrace,
    bool& Success,
    FHitResult& HitResult) {
  HitResult = FHitResult();
  Success = false;

  UWorld* world = this->GetWorld();

  FViewport* pViewport = GEditor->GetActiveViewport();
  FViewportClient* pViewportClient = pViewport->GetClient();
  FEditorViewportClient* pEditorViewportClient =
      static_cast<FEditorViewportClient*>(pViewportClient);

  if (!world || !pEditorViewportClient ||
      !pEditorViewportClient->Viewport->HasFocus()) {
    return;
  }

  FViewportCursorLocation cursor =
      pEditorViewportClient->GetCursorWorldLocationFromMousePos();

  const FVector& viewLoc = cursor.GetOrigin();
  const FVector& viewDir = cursor.GetDirection();

  FVector lineEnd = viewLoc + viewDir * 637100000.0;

  static const FName LineTraceSingleName(TEXT("LevelEditorLineTrace"));
  if (ShowTrace) {
    world->DebugDrawTraceTag = LineTraceSingleName;
  } else {
    world->DebugDrawTraceTag = NAME_None;
  }

  FCollisionQueryParams CollisionParams(LineTraceSingleName);

  FCollisionObjectQueryParams ObjectParams =
      FCollisionObjectQueryParams(ECC_WorldStatic);
  ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
  ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
  ObjectParams.AddObjectTypesToQuery(ECC_Visibility);

  if (world->LineTraceSingleByObjectType(
          HitResult,
          viewLoc,
          lineEnd,
          ObjectParams,
          CollisionParams)) {
    Success = true;
  }
}
#endif