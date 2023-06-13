// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGeoreference.h"
#include "Camera/PlayerCameraManager.h"
#include "CesiumActors.h"
#include "CesiumCommon.h"
#include "CesiumRuntime.h"
#include "CesiumSubLevelComponent.h"
#include "CesiumSubLevelInstance.h"
#include "CesiumSubLevelSwitcherComponent.h"
#include "CesiumTransforms.h"
#include "CesiumUtility/Math.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "Engine/WorldComposition.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GeoTransforms.h"
#include "Math/Matrix.h"
#include "Math/RotationTranslationMatrix.h"
#include "Math/Rotator.h"
#include "Math/Vector.h"
#include "Misc/PackageName.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "VecMath.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/quaternion.hpp>
#include <optional>
#include <string>

#if WITH_EDITOR
#include "DrawDebugHelpers.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "Slate/SceneViewport.h"
#include "WorldBrowserModule.h"

// These are in the Private directory, yet they are exported, so we're able to
// use them. And there's no other way (AFAIK) to get details of unloaded levels.
#include "../Private/LevelCollectionModel.h"
#include "../Private/LevelModel.h"
#endif

FName ACesiumGeoreference::DEFAULT_GEOREFERENCE_TAG =
    FName("DEFAULT_GEOREFERENCE");

/*static*/ ACesiumGeoreference*
ACesiumGeoreference::GetDefaultGeoreference(const UObject* WorldContextObject) {
  UWorld* world = WorldContextObject->GetWorld();
  // This method can be called by actors even when opening the content browser.
  if (!IsValid(world)) {
    return nullptr;
  }
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("World name for GetDefaultGeoreference: %s"),
      *world->GetFullName());

  // Note: The actor iterator will be created with the
  // "EActorIteratorFlags::SkipPendingKill" flag,
  // meaning that we don't have to handle objects
  // that have been deleted. (This is the default,
  // but made explicit here)
  ACesiumGeoreference* pGeoreference = nullptr;
  EActorIteratorFlags flags = EActorIteratorFlags::OnlyActiveLevels |
                              EActorIteratorFlags::SkipPendingKill;
  for (TActorIterator<AActor> actorIterator(
           world,
           ACesiumGeoreference::StaticClass(),
           flags);
       actorIterator;
       ++actorIterator) {
    AActor* actor = *actorIterator;
    if (actor->ActorHasTag(DEFAULT_GEOREFERENCE_TAG)) {
      pGeoreference = Cast<ACesiumGeoreference>(actor);
      break;
    }
  }
  if (!pGeoreference) {
    // Legacy method of finding Georeference, for backwards compatibility with
    // existing projects
    ACesiumGeoreference* pGeoreferenceCandidate =
        FindObject<ACesiumGeoreference>(
            world->PersistentLevel,
            TEXT("CesiumGeoreferenceDefault"));
    // Test if PendingKill
    if (IsValid(pGeoreferenceCandidate)) {
      pGeoreference = pGeoreferenceCandidate;
    }
  }
  if (!pGeoreference) {
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("Creating default Georeference for actor %s"),
        *WorldContextObject->GetName());
    // Spawn georeference in the persistent level
    FActorSpawnParameters spawnParameters;
    spawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    pGeoreference = world->SpawnActor<ACesiumGeoreference>(spawnParameters);
    // Null check so the editor doesn't crash when it makes arbitrary calls to
    // this function without a valid world context object.
    if (pGeoreference) {
      pGeoreference->Tags.Add(DEFAULT_GEOREFERENCE_TAG);
    }

  } else {
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("Using existing Georeference %s for actor %s"),
        *pGeoreference->GetName(),
        *WorldContextObject->GetName());
  }
  return pGeoreference;
}

ACesiumGeoreference::ACesiumGeoreference()
    : _ellipsoidRadii{
        CesiumGeospatial::Ellipsoid::WGS84.getRadii().x,
        CesiumGeospatial::Ellipsoid::WGS84.getRadii().y,
        CesiumGeospatial::Ellipsoid::WGS84.getRadii().z},
      _geoTransforms(),
      _insideSublevel(false) {
  PrimaryActorTick.bCanEverTick = true;

  this->SubLevelSwitcher =
      CreateDefaultSubobject<UCesiumSubLevelSwitcherComponent>(
          "SubLevelSwitcher");
}

#if WITH_EDITOR

void ACesiumGeoreference::PlaceGeoreferenceOriginHere() {
  // If this is PIE mode, ignore
  UWorld* pWorld = this->GetWorld();
  if (!GEditor || pWorld->IsGameWorld()) {
    return;
  }

  FViewport* pViewport = GEditor->GetActiveViewport();
  FViewportClient* pViewportClient = pViewport->GetClient();
  FEditorViewportClient* pEditorViewportClient =
      static_cast<FEditorViewportClient*>(pViewportClient);

  FRotationTranslationMatrix fCameraTransform(
      pEditorViewportClient->GetViewRotation(),
      pEditorViewportClient->GetViewLocation());
  const FIntVector& originLocation = pWorld->OriginLocation;

  // TODO: optimize this, only need to transform the front direction and
  // translation

  const FVector& viewLocation = pEditorViewportClient->GetViewLocation();
  glm::dvec4 translation = VecMath::add4D(viewLocation, originLocation);

  // camera local space to Unreal absolute world
  glm::dmat4 cameraToAbsolute =
      VecMath::createMatrix4D(fCameraTransform, translation);

  // camera local space to ECEF
  glm::dmat4 cameraToECEF =
      this->_geoTransforms
          .GetAbsoluteUnrealWorldToEllipsoidCenteredTransform() *
      cameraToAbsolute;

  // Long/Lat/Height camera location, in degrees/meters (also our new target
  // georeference origin) When the location is too close to the center of the
  // earth, the result will be (0,0,0)
  glm::dvec3 targetGeoreferenceOrigin =
      _geoTransforms.TransformEcefToLongitudeLatitudeHeight(
          glm::dvec3(cameraToECEF[3]));

  this->_setGeoreferenceOrigin(
      targetGeoreferenceOrigin.x,
      targetGeoreferenceOrigin.y,
      targetGeoreferenceOrigin.z);

  glm::dmat4 absoluteToRelativeWorld = VecMath::createTranslationMatrix4D(
      -originLocation.X,
      -originLocation.Y,
      -originLocation.Z,
      1.0);

  // TODO: check for degeneracy ?
  glm::dmat4 newCameraTransform =
      absoluteToRelativeWorld *
      this->_geoTransforms
          .GetEllipsoidCenteredToAbsoluteUnrealWorldTransform() *
      cameraToECEF;
  glm::dvec3 cameraFront = glm::normalize(glm::dvec3(newCameraTransform[0]));
  glm::dvec3 cameraRight =
      glm::normalize(glm::cross(glm::dvec3(0.0, 0.0, 1.0), cameraFront));
  glm::dvec3 cameraUp = glm::normalize(glm::cross(cameraFront, cameraRight));

  pEditorViewportClient->SetViewRotation(
      FMatrix(
          FVector(cameraFront.x, cameraFront.y, cameraFront.z),
          FVector(cameraRight.x, cameraRight.y, cameraRight.z),
          FVector(cameraUp.x, cameraUp.y, cameraUp.z),
          FVector::ZeroVector)
          .Rotator());
  pEditorViewportClient->SetViewLocation(
      FVector(-originLocation.X, -originLocation.Y, -originLocation.Z));

  this->_enableAndGeoreferenceCurrentSubLevel();
}

#endif

namespace {

template <typename TStringish>
FString longPackageNameToCesiumName(UWorld* pWorld, const TStringish& name) {
  FString levelName = FPackageName::GetShortName(name);
  levelName.RemoveFromStart(pWorld->StreamingLevelsPrefix);
  return levelName;
}

} // namespace

#if WITH_EDITOR
void ACesiumGeoreference::_updateCesiumSubLevels() {
  UWorld* pWorld = this->GetWorld();
  if (!IsValid(pWorld)) {
    // This happens for the georeference that is shown in the
    // content browser. Might omit this message.
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT(
            "Georeference is not spawned in world: %s, skipping _updateCesiumSubLevels"),
        *this->GetFullName());
    return;
  }

  if (this->CesiumSubLevels_DEPRECATED.IsEmpty() ||
      !this->_shouldManageSubLevels())
    return;

  this->Modify();

  // Convert old-style sublevels (using World Composition) to new style
  // sub-levels (level instances)
  UWorldComposition::FTilesList& allLevels =
      pWorld->WorldComposition->GetTilesList();

  for (const FWorldCompositionTile& level : allLevels) {
    FString levelName = longPackageNameToCesiumName(pWorld, level.PackageName);

    // Check if the level is already known
    FCesiumSubLevel* pFound = this->CesiumSubLevels_DEPRECATED.FindByPredicate(
        [&levelName](FCesiumSubLevel& subLevel) {
          return levelName.Equals(subLevel.LevelName);
        });

    if (pFound == nullptr || !pFound->CanBeEnabled)
      continue;

    FActorSpawnParameters spawnParameters{};
    spawnParameters.Name = FName(pFound->LevelName);
    spawnParameters.ObjectFlags = RF_Transactional;

    ALevelInstance* pLevelInstance = pWorld->SpawnActor<ALevelInstance>(
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        spawnParameters);
    pLevelInstance->SetActorLabel(pFound->LevelName);

    FString levelPath = level.PackageName.ToString() + "." +
                        FPackageName::GetShortName(level.PackageName);
    TSoftObjectPtr<UWorld> asset{FSoftObjectPath(levelPath)};
    pLevelInstance->SetWorldAsset(asset);

    UCesiumSubLevelComponent* pLevelComponent =
        Cast<UCesiumSubLevelComponent>(pLevelInstance->AddComponentByClass(
            UCesiumSubLevelComponent::StaticClass(),
            false,
            FTransform::Identity,
            false));
    pLevelComponent->SetFlags(RF_Transactional);
    pLevelInstance->AddInstanceComponent(pLevelComponent);

    pLevelComponent->SetOriginLongitudeLatitudeHeight(FVector(
        pFound->LevelLongitude,
        pFound->LevelLatitude,
        pFound->LevelHeight));
    pLevelComponent->SetEnabled(pFound->Enabled);
    pLevelComponent->SetLoadRadius(pFound->LoadRadius);

    pLevelInstance->LoadLevelInstance();
  }

  this->CesiumSubLevels_DEPRECATED.Empty();
}
#endif

bool ACesiumGeoreference::SwitchToLevel(int32 Index) { return false; }

FVector
ACesiumGeoreference::GetGeoreferenceOriginLongitudeLatitudeHeight() const {
  return FVector(OriginLongitude, OriginLatitude, OriginHeight);
}

void ACesiumGeoreference::SetGeoreferenceOriginLongitudeLatitudeHeight(
    const glm::dvec3& targetLongitudeLatitudeHeight) {
  // Should not allow externally initiated georeference origin changing if we
  // are inside a sublevel
  // if (this->_insideSublevel) {
  //  return;
  //}
  this->_setGeoreferenceOrigin(
      targetLongitudeLatitudeHeight.x,
      targetLongitudeLatitudeHeight.y,
      targetLongitudeLatitudeHeight.z);
}

void ACesiumGeoreference::SetGeoreferenceOrigin(
    const glm::dvec3& TargetLongitudeLatitudeHeight) {
  UE_LOG(
      LogCesium,
      Warning,
      TEXT(
          "SetGeoreferenceOrigin was renamed to SetGeoreferenceOriginLongitudeLatitudeHeight."));
  SetGeoreferenceOriginLongitudeLatitudeHeight(TargetLongitudeLatitudeHeight);
}

void ACesiumGeoreference::SetGeoreferenceOriginEcef(
    const glm::dvec3& TargetEcef) {
  SetGeoreferenceOriginLongitudeLatitudeHeight(
      _geoTransforms.TransformEcefToLongitudeLatitudeHeight(TargetEcef));
}

void ACesiumGeoreference::SetGeoreferenceOriginLongitudeLatitudeHeight(
    const FVector& targetLongitudeLatitudeHeight) {
  this->SetGeoreferenceOriginLongitudeLatitudeHeight(glm::dvec3(
      targetLongitudeLatitudeHeight.X,
      targetLongitudeLatitudeHeight.Y,
      targetLongitudeLatitudeHeight.Z));
}

void ACesiumGeoreference::SetGeoreferenceOriginEcef(const FVector& TargetEcef) {
  this->SetGeoreferenceOriginEcef(
      glm::dvec3(TargetEcef.X, TargetEcef.Y, TargetEcef.Z));
}

// Called when the game starts or when spawned
void ACesiumGeoreference::BeginPlay() {
  Super::BeginPlay();

  PrimaryActorTick.TickGroup = TG_PrePhysics;

  UWorld* pWorld = this->GetWorld();
  if (!pWorld) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreference does not have a World in BeginPlay."));
    return;
  }

  if (!this->SubLevelCamera) {
    // Find the first player's camera manager
    APlayerController* pPlayerController = pWorld->GetFirstPlayerController();
    if (pPlayerController) {
      this->SubLevelCamera = pPlayerController->PlayerCameraManager;
    }

    if (!this->SubLevelCamera) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "CesiumGeoreference could not find a FirstPlayerController or a corresponding PlayerCameraManager."));
    }
  }

  UpdateGeoreference();
}

void ACesiumGeoreference::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  Super::EndPlay(EndPlayReason);
}

/** In case the CesiumGeoreference gets spawned at run time, instead of design
 *  time, ensure that frames are updated */
void ACesiumGeoreference::OnConstruction(const FTransform& Transform) {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called OnConstruction on actor %s"),
      *this->GetName());

  this->UpdateGeoreference();
}

void ACesiumGeoreference::BeginDestroy() { Super::BeginDestroy(); }

void ACesiumGeoreference::PostLoad() { Super::PostLoad(); }

void ACesiumGeoreference::UpdateGeoreference() {
  this->_updateGeoTransforms();

  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Broadcasting OnGeoreferenceUpdated for Georeference %s"),
      *this->GetFullName());

  OnGeoreferenceUpdated.Broadcast();
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
          GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, OriginHeight)) {
    this->_enableAndGeoreferenceCurrentSubLevel();
    this->UpdateGeoreference();
    return;
  } else if (
      propertyName == GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, Scale)) {
    if (this->Scale < 1e-6) {
      this->Scale = 1e-6;
    }
    this->UpdateGeoreference();
  }
}
#endif

bool ACesiumGeoreference::ShouldTickIfViewportsOnly() const { return true; }

#if WITH_EDITOR
void ACesiumGeoreference::_showSubLevelLoadRadii() const {
  UWorld* world = this->GetWorld();
  if (world->IsGameWorld()) {
    return;
  }
  if (!this->ShowLoadRadii) {
    return;
  }
  const glm::dvec4 originLocation =
      glm::dvec4(VecMath::createVector3D(world->OriginLocation), 1.0);
  for (const auto& pLevelWeak :
       this->SubLevelSwitcher->GetRegisteredSubLevels()) {
    ALevelInstance* pLevel = pLevelWeak.Get();
    if (!IsValid(pLevel))
      continue;

    UCesiumSubLevelComponent* pComponent =
        pLevel->FindComponentByClass<UCesiumSubLevelComponent>();

    glm::dvec3 levelECEF =
        _geoTransforms.TransformLongitudeLatitudeHeightToEcef(glm::dvec3(
            pComponent->GetOriginLongitude(),
            pComponent->GetOriginLatitude(),
            pComponent->GetOriginHeight()));

    glm::dvec4 levelAbs =
        this->_geoTransforms
            .GetEllipsoidCenteredToAbsoluteUnrealWorldTransform() *
        glm::dvec4(levelECEF, 1.0);
    FVector levelRelative = VecMath::createVector(levelAbs - originLocation);
    DrawDebugSphere(
        world,
        levelRelative,
        100.0 * pComponent->GetLoadRadius(),
        100,
        FColor::Blue);
  }
}
#endif // WITH_EDITOR

#if WITH_EDITOR
void ACesiumGeoreference::_handleViewportOriginEditing() {
  if (!this->EditOriginInViewport) {
    return;
  }
  FHitResult mouseRayResults;
  bool mouseRaySuccess;

  this->_lineTraceViewportMouse(false, mouseRaySuccess, mouseRayResults);

  if (!mouseRaySuccess) {
    return;
  }
  const FIntVector& originLocation = this->GetWorld()->OriginLocation;

  FVector grabbedLocation = mouseRayResults.Location;
  // convert from UE to ECEF to LongitudeLatitudeHeight
  glm::dvec4 grabbedLocationAbs =
      VecMath::add4D(grabbedLocation, originLocation);

  glm::dvec3 grabbedLocationECEF = glm::dvec3(
      this->_geoTransforms
          .GetAbsoluteUnrealWorldToEllipsoidCenteredTransform() *
      grabbedLocationAbs);

  glm::dvec3 cartographic =
      _geoTransforms.TransformEcefToLongitudeLatitudeHeight(
          grabbedLocationECEF);

  UE_LOG(
      LogActor,
      Warning,
      TEXT("Mouse Location: (Longitude: %f, Latitude: %f, Height: %f)"),
      cartographic.x,
      cartographic.y,
      cartographic.z);

  // TODO: find editor viewport mouse click event
  // if (mouseDown) {
  // this->_setGeoreferenceOrigin()
  //	this->EditOriginInViewport = false;
  //}
}
#endif // WITH_EDITOR

namespace {
/**
 * @brief Clamping addition.
 *
 * Returns the sum of the given values, clamping the result to
 * the minimum/maximum value that can be represented as a 32 bit
 * signed integer.
 *
 * @param f The floating point value
 * @param i The integer value
 * @return The clamped result
 */
int32 clampedAdd(double f, int32 i) {
  int64 sum = static_cast<int64>(f) + static_cast<int64>(i);
  int64 min = static_cast<int64>(TNumericLimits<int32>::Min());
  int64 max = static_cast<int64>(TNumericLimits<int32>::Max());
  int64 clamped = FMath::Max(min, FMath::Min(max, sum));
  return static_cast<int32>(clamped);
}
} // namespace

bool ACesiumGeoreference::_updateSublevelState() {
  const TArray<TWeakObjectPtr<ALevelInstance>>& sublevels =
      this->SubLevelSwitcher->GetRegisteredSubLevels();

  if (sublevels.Num() == 0) {
    // If we don't have any known sublevels, bail quickly to save ourselves a
    // little work.
    return false;
  }

  if (!IsValid(this->SubLevelCamera)) {
    return false;
  }

  UWorld* pWorld = this->GetWorld();
  const FIntVector& originLocation = pWorld->OriginLocation;
  const FMinimalViewInfo& pov = this->SubLevelCamera->ViewTarget.POV;
  const FVector& cameraLocation = pov.Location;

  glm::dvec4 cameraAbsolute = VecMath::add4D(cameraLocation, originLocation);

  glm::dvec3 cameraECEF = glm::dvec3(
      this->_geoTransforms
          .GetAbsoluteUnrealWorldToEllipsoidCenteredTransform() *
      cameraAbsolute);

  ALevelInstance* pClosestActiveLevel = nullptr;
  double closestLevelDistance = std::numeric_limits<double>::max();

  for (int32 i = 0; i < sublevels.Num(); ++i) {
    ALevelInstance* pCurrent = sublevels[i].Get();
    if (!IsValid(pCurrent))
      continue;

    UCesiumSubLevelComponent* pComponent =
        pCurrent->FindComponentByClass<UCesiumSubLevelComponent>();
    if (!IsValid(pComponent))
      continue;

    if (!pComponent->GetEnabled())
      continue;

    glm::dvec3 levelECEF =
        _geoTransforms.TransformLongitudeLatitudeHeightToEcef(glm::dvec3(
            pComponent->GetOriginLongitude(),
            pComponent->GetOriginLatitude(),
            pComponent->GetOriginHeight()));

    double levelDistance = glm::length(levelECEF - cameraECEF);
    if (levelDistance < pComponent->GetLoadRadius() &&
        levelDistance < closestLevelDistance) {
      pClosestActiveLevel = pCurrent;
      closestLevelDistance = levelDistance;
    }
  }

  this->SubLevelSwitcher->SetTarget(pClosestActiveLevel);

  return pClosestActiveLevel != nullptr;
}

void ACesiumGeoreference::_updateGeoTransforms() {
  glm::dvec3 center(0.0, 0.0, 0.0);
  if (this->OriginPlacement == EOriginPlacement::CartographicOrigin) {
    center = _geoTransforms.TransformLongitudeLatitudeHeightToEcef(glm::dvec3(
        this->OriginLongitude,
        this->OriginLatitude,
        this->OriginHeight));
  }

  this->_geoTransforms = GeoTransforms(
      CesiumGeospatial::Ellipsoid(
          this->_ellipsoidRadii[0],
          this->_ellipsoidRadii[1],
          this->_ellipsoidRadii[2]),
      center,
      this->Scale / 100.0);
}

void ACesiumGeoreference::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);

#if WITH_EDITOR
  // There doesn't appear to be a good way to be notified about the wide variety
  // of level manipulations we care about, so in the Editor we'll poll.
  if (GEditor && IsValid(this->GetWorld()) &&
      IsValid(this->GetWorld()->WorldComposition) &&
      !this->GetWorld()->IsGameWorld()) {
    this->_updateCesiumSubLevels();
  }

  _showSubLevelLoadRadii();
  _handleViewportOriginEditing();
#endif

  if (this->_shouldManageSubLevels()) {
    this->_insideSublevel = _updateSublevelState();
  }
}

void ACesiumGeoreference::Serialize(FArchive& Ar) {
  Super::Serialize(Ar);

  // Recompute derived values on load.
  if (Ar.IsLoading()) {
    this->_updateGeoTransforms();
  }
}

/**
 * Useful Conversion Functions
 */

glm::dvec3 ACesiumGeoreference::TransformLongitudeLatitudeHeightToEcef(
    const glm::dvec3& longitudeLatitudeHeight) const {
  return _geoTransforms.TransformLongitudeLatitudeHeightToEcef(
      longitudeLatitudeHeight);
}

FVector ACesiumGeoreference::TransformLongitudeLatitudeHeightToEcef(
    const FVector& longitudeLatitudeHeight) const {
  glm::dvec3 ecef = this->_geoTransforms.TransformLongitudeLatitudeHeightToEcef(
      VecMath::createVector3D(longitudeLatitudeHeight));
  return FVector(ecef.x, ecef.y, ecef.z);
}

glm::dvec3 ACesiumGeoreference::TransformEcefToLongitudeLatitudeHeight(
    const glm::dvec3& ecef) const {
  return _geoTransforms.TransformEcefToLongitudeLatitudeHeight(ecef);
}

FVector ACesiumGeoreference::TransformEcefToLongitudeLatitudeHeight(
    const FVector& ecef) const {
  glm::dvec3 llh = this->_geoTransforms.TransformEcefToLongitudeLatitudeHeight(
      glm::dvec3(ecef.X, ecef.Y, ecef.Z));
  return FVector(llh.x, llh.y, llh.z);
}

glm::dvec3 ACesiumGeoreference::TransformLongitudeLatitudeHeightToUnreal(
    const glm::dvec3& longitudeLatitudeHeight) const {
  return this->_geoTransforms.TransformLongitudeLatitudeHeightToUnreal(
      glm::dvec3(CesiumActors::getWorldOrigin4D(this)),
      longitudeLatitudeHeight);
}

FVector ACesiumGeoreference::TransformLongitudeLatitudeHeightToUnreal(
    const FVector& longitudeLatitudeHeight) const {
  glm::dvec3 ue = this->_geoTransforms.TransformLongitudeLatitudeHeightToUnreal(
      glm::dvec3(CesiumActors::getWorldOrigin4D(this)),
      VecMath::createVector3D(longitudeLatitudeHeight));
  return FVector(ue.x, ue.y, ue.z);
}

glm::dvec3 ACesiumGeoreference::TransformUnrealToLongitudeLatitudeHeight(
    const glm::dvec3& ue) const {
  return this->_geoTransforms.TransformUnrealToLongitudeLatitudeHeight(
      glm::dvec3(CesiumActors::getWorldOrigin4D(this)),
      ue);
}

FVector ACesiumGeoreference::TransformUnrealToLongitudeLatitudeHeight(
    const FVector& ue) const {
  glm::dvec3 llh =
      this->_geoTransforms.TransformUnrealToLongitudeLatitudeHeight(
          glm::dvec3(CesiumActors::getWorldOrigin4D(this)),
          VecMath::createVector3D(ue));
  return FVector(llh.x, llh.y, llh.z);
}

glm::dvec3
ACesiumGeoreference::TransformEcefToUnreal(const glm::dvec3& ecef) const {
  return this->_geoTransforms.TransformEcefToUnreal(
      glm::dvec3(CesiumActors::getWorldOrigin4D(this)),
      ecef);
}

FVector ACesiumGeoreference::TransformEcefToUnreal(const FVector& ecef) const {
  glm::dvec3 ue = this->_geoTransforms.TransformEcefToUnreal(
      glm::dvec3(CesiumActors::getWorldOrigin4D(this)),
      VecMath::createVector3D(ecef));
  return FVector(ue.x, ue.y, ue.z);
}

glm::dvec3
ACesiumGeoreference::TransformUnrealToEcef(const glm::dvec3& ue) const {
  return this->_geoTransforms.TransformUnrealToEcef(
      glm::dvec3(CesiumActors::getWorldOrigin4D(this)),
      ue);
}

FVector ACesiumGeoreference::TransformUnrealToEcef(const FVector& ue) const {
  glm::dvec3 ecef = this->_geoTransforms.TransformUnrealToEcef(
      glm::dvec3(CesiumActors::getWorldOrigin4D(this)),
      glm::dvec3(ue.X, ue.Y, ue.Z));
  return FVector(ecef.x, ecef.y, ecef.z);
}

glm::dquat ACesiumGeoreference::TransformRotatorUnrealToEastSouthUp(
    const glm::dquat& UeRotator,
    const glm::dvec3& UeLocation) const {
  return this->_geoTransforms.TransformRotatorUnrealToEastSouthUp(
      glm::dvec3(CesiumActors::getWorldOrigin4D(this)),
      UeRotator,
      UeLocation);
}

FRotator ACesiumGeoreference::TransformRotatorUnrealToEastSouthUp(
    const FRotator& UERotator,
    const FVector& ueLocation) const {
  glm::dquat q = TransformRotatorUnrealToEastSouthUp(
      VecMath::createQuaternion(UERotator.Quaternion()),
      VecMath::createVector3D(ueLocation));
  return VecMath::createRotator(q);
}

glm::dquat ACesiumGeoreference::TransformRotatorEastSouthUpToUnreal(
    const glm::dquat& EsuRotator,
    const glm::dvec3& UeLocation) const {
  return this->_geoTransforms.TransformRotatorEastSouthUpToUnreal(
      glm::dvec3(CesiumActors::getWorldOrigin4D(this)),
      EsuRotator,
      UeLocation);
}

FRotator ACesiumGeoreference::TransformRotatorEastSouthUpToUnreal(
    const FRotator& EsuRotator,
    const FVector& ueLocation) const {
  glm::dquat q = TransformRotatorEastSouthUpToUnreal(
      VecMath::createQuaternion(EsuRotator.Quaternion()),
      VecMath::createVector3D(ueLocation));
  return VecMath::createRotator(q);
}

glm::dmat3
ACesiumGeoreference::ComputeEastSouthUpToUnreal(const glm::dvec3& ue) const {
  return _geoTransforms.ComputeEastSouthUpToUnreal(
      glm::dvec3(CesiumActors::getWorldOrigin4D(this)),
      ue);
}

FMatrix
ACesiumGeoreference::ComputeEastSouthUpToUnreal(const FVector& ue) const {
  glm::dmat3 esuToUe = this->_geoTransforms.ComputeEastSouthUpToUnreal(
      glm::dvec3(CesiumActors::getWorldOrigin4D(this)),
      glm::dvec3(ue.X, ue.Y, ue.Z));
  return VecMath::createMatrix(esuToUe);
}

glm::dmat3
ACesiumGeoreference::ComputeEastNorthUpToEcef(const glm::dvec3& ecef) const {
  return _geoTransforms.ComputeEastNorthUpToEcef(ecef);
}

FMatrix
ACesiumGeoreference::ComputeEastNorthUpToEcef(const FVector& ecef) const {
  glm::dmat3 enuToEcef = this->_geoTransforms.ComputeEastNorthUpToEcef(
      glm::dvec3(ecef.X, ecef.Y, ecef.Z));
  return VecMath::createMatrix(enuToEcef);
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

// TODO: should consider raycasting the WGS84 ellipsoid instead. The Unreal
// raycast seems to be inaccurate at glancing angles, perhaps due to the large
// single-precision distances.
#if WITH_EDITOR
void ACesiumGeoreference::_lineTraceViewportMouse(
    const bool ShowTrace,
    bool& Success,
    FHitResult& HitResult) const {
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

  // TODO (low prio, because this whole function is preliminary) :
  // This radius should probably be taken from the ellipsoid
  const double earthRadiusCm = 637100000.0;
  FVector lineEnd = viewLoc + viewDir * earthRadiusCm;

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

FCesiumSubLevel* ACesiumGeoreference::_findCesiumSubLevelByName(
    const FName& packageName,
    bool createIfDoesNotExist) {
  FString cesiumName =
      longPackageNameToCesiumName(this->GetWorld(), packageName);

  FCesiumSubLevel* pCesiumLevel =
      this->CesiumSubLevels_DEPRECATED.FindByPredicate(
          [cesiumName](const FCesiumSubLevel& level) {
            return cesiumName == level.LevelName;
          });

  if (!pCesiumLevel && createIfDoesNotExist) {
    // No Cesium sub-level exists, so create it now.
    this->CesiumSubLevels_DEPRECATED.Add(FCesiumSubLevel{
        cesiumName,
        true,
        this->OriginLatitude,
        this->OriginLongitude,
        this->OriginHeight,
        1000.0,
        true});
    pCesiumLevel = &this->CesiumSubLevels_DEPRECATED.Last();
  }

  return pCesiumLevel;
}

#if WITH_EDITOR

void ACesiumGeoreference::_enableAndGeoreferenceCurrentSubLevel() {
  // If a sub-level is the current one, enable it and also update the
  // sub-level's location.
  ULevel* pCurrent = this->GetWorld()->GetCurrentLevel();
  if (!pCurrent || pCurrent->IsPersistentLevel()) {
    return;
  }

  UPackage* pLevelPackage = pCurrent->GetOutermost();
  FCesiumSubLevel* pLevel =
      this->_findCesiumSubLevelByName(pLevelPackage->GetFName(), false);

  if (pLevel) {
    pLevel->LevelLongitude = this->OriginLongitude;
    pLevel->LevelLatitude = this->OriginLatitude;
    pLevel->LevelHeight = this->OriginHeight;

    pLevel->Enabled = pLevel->CanBeEnabled;
  }
}

#endif

bool ACesiumGeoreference::_shouldManageSubLevels() const {
  // Only a Georeference in the PersistentLevel should manage sub-levels.
  return this->GetLevel()->IsPersistentLevel();
}
