// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGeoreference.h"
#include "Camera/PlayerCameraManager.h"
#include "CesiumActors.h"
#include "CesiumGeospatial/Transforms.h"
#include "CesiumRuntime.h"
#include "CesiumTransforms.h"
#include "CesiumUtility/Math.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
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
}

void ACesiumGeoreference::PostInitProperties() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called PostInitProperties on actor %s"),
      *this->GetName());

  Super::PostInitProperties();

  // Initialize the GeoTransforms with the state from the
  // deserialized properties
  _geoTransforms.setEllipsoid(CesiumGeospatial::Ellipsoid(
      glm::dvec3(_ellipsoidRadii[0], _ellipsoidRadii[1], _ellipsoidRadii[2])));
  UpdateGeoreference();
}

void ACesiumGeoreference::PlaceGeoreferenceOriginHere() {
#if WITH_EDITOR
  // TODO: should we just assume origin rebasing isn't happening since this is
  // only editor-mode?

  // If this is PIE mode, ignore
  UWorld* world = this->GetWorld();
  if (world->IsGameWorld()) {
    return;
  }

  FViewport* pViewport = GEditor->GetActiveViewport();
  FViewportClient* pViewportClient = pViewport->GetClient();
  FEditorViewportClient* pEditorViewportClient =
      static_cast<FEditorViewportClient*>(pViewportClient);

  FRotationTranslationMatrix fCameraTransform(
      pEditorViewportClient->GetViewRotation(),
      pEditorViewportClient->GetViewLocation());
  const FIntVector& originLocation = world->OriginLocation;

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
      _geoTransforms.TransformEcefToLongitudeLatitudeHeight(cameraToECEF[3]);

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
  glm::dvec3 cameraFront = glm::normalize(newCameraTransform[0]);
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
#endif
}

void ACesiumGeoreference::CheckForNewSubLevels() { _updateCesiumSubLevels(); }

void ACesiumGeoreference::_updateCesiumSubLevels() {
  UWorld* world = this->GetWorld();
  if (!IsValid(world)) {
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

  // Update the array of sublevels, based on the current
  // streaming levels of the world: Add all levels that
  // are not yet contained in the array.
  // (Levels that are not found in the streaming levels
  // are NOT removed, because they might just be unloaded
  // and not really deleted)
  const TArray<ULevelStreaming*>& streamedLevels = world->GetStreamingLevels();
  for (ULevelStreaming* streamedLevel : streamedLevels) {
    FString levelName =
        FPackageName::GetShortName(streamedLevel->GetWorldAssetPackageName());
    levelName.RemoveFromStart(world->StreamingLevelsPrefix);
    // Check if the level is already known
    bool found = false;
    for (FCesiumSubLevel& subLevel : this->CesiumSubLevels) {
      if (levelName.Equals(subLevel.LevelName)) {
        found = true;
        break;
      }
    }
    // If the level was not known yet, create a new one
    if (!found) {
      CesiumSubLevels.Add(FCesiumSubLevel{
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

FVector
ACesiumGeoreference::InaccurateGetGeoreferenceOriginLongitudeLatitudeHeight()
    const {
  return FVector(OriginLongitude, OriginLatitude, OriginHeight);
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
  UpdateGeoreference();
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

void ACesiumGeoreference::UpdateGeoreference() {
  glm::dvec3 center(0.0, 0.0, 0.0);
  if (this->OriginPlacement == EOriginPlacement::CartographicOrigin) {
    center = _geoTransforms.TransformLongitudeLatitudeHeightToEcef(glm::dvec3(
        this->OriginLongitude,
        this->OriginLatitude,
        this->OriginHeight));
  }
  _geoTransforms.setCenter(center);

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
  for (const FCesiumSubLevel& level : this->CesiumSubLevels) {
    glm::dvec3 levelECEF =
        _geoTransforms.TransformLongitudeLatitudeHeightToEcef(glm::dvec3(
            level.LevelLongitude,
            level.LevelLatitude,
            level.LevelHeight));

    glm::dvec4 levelAbs =
        this->_geoTransforms
            .GetEllipsoidCenteredToAbsoluteUnrealWorldTransform() *
        glm::dvec4(levelECEF, 1.0);
    FVector levelRelative = VecMath::createVector(levelAbs - originLocation);
    DrawDebugSphere(
        world,
        levelRelative,
        100.0 * level.LoadRadius,
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

  glm::dvec3 grabbedLocationECEF =
      this->_geoTransforms
          .GetAbsoluteUnrealWorldToEllipsoidCenteredTransform() *
      grabbedLocationAbs;

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
int32 clampedAdd(float f, int32 i) {
  int64 sum = static_cast<int64>(f) + static_cast<int64>(i);
  int64 min = static_cast<int64>(TNumericLimits<int32>::Min());
  int64 max = static_cast<int64>(TNumericLimits<int32>::Max());
  int64 clamped = FMath::Max(min, FMath::Min(max, sum));
  return static_cast<int32>(clamped);
}
} // namespace

bool ACesiumGeoreference::_updateSublevelState() {

  if (!IsValid(WorldOriginCamera)) {
    return false;
  }

  UWorld* world = this->GetWorld();
  const FIntVector& originLocation = world->OriginLocation;
  const FMinimalViewInfo& pov = this->WorldOriginCamera->ViewTarget.POV;
  const FVector& cameraLocation = pov.Location;

  glm::dvec4 cameraAbsolute = VecMath::add4D(cameraLocation, originLocation);

  glm::dvec3 cameraECEF =
      this->_geoTransforms
          .GetAbsoluteUnrealWorldToEllipsoidCenteredTransform() *
      cameraAbsolute;

  bool isInsideSublevel = false;

  const TArray<ULevelStreaming*>& streamedLevels = world->GetStreamingLevels();
  for (ULevelStreaming* streamedLevel : streamedLevels) {
    FString levelName =
        FPackageName::GetShortName(streamedLevel->GetWorldAssetPackageName());
    levelName.RemoveFromStart(world->StreamingLevelsPrefix);
    // TODO: maybe we should precalculate the level ECEF from level
    // long/lat/height
    // TODO: consider the case where we're intersecting multiple level radii
    for (FCesiumSubLevel& level : this->CesiumSubLevels) {
      // if this is a known level, we need to tell it whether or not it should
      // be loaded
      if (levelName.Equals(level.LevelName)) {
        glm::dvec3 levelECEF =
            _geoTransforms.TransformLongitudeLatitudeHeightToEcef(glm::dvec3(
                level.LevelLongitude,
                level.LevelLatitude,
                level.LevelHeight));

        if (glm::length(levelECEF - cameraECEF) < level.LoadRadius) {
          isInsideSublevel = true;
          if (!level.CurrentlyLoaded) {
            this->_jumpToLevel(level);
            streamedLevel->SetShouldBeLoaded(true);
            streamedLevel->SetShouldBeVisible(true);
            level.CurrentlyLoaded = true;
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
  return isInsideSublevel;
}

void ACesiumGeoreference::_performOriginRebasing() {
  UWorld* world = this->GetWorld();
  if (!world->IsGameWorld()) {
    return;
  }
  if (!IsValid(WorldOriginCamera)) {
    return;
  }
  if (!this->KeepWorldOriginNearCamera) {
    return;
  }
  if (this->_insideSublevel && !this->OriginRebaseInsideSublevels) {
    // If we are not going to continue origin rebasing inside the
    // sublevel, just set the origin back to zero if necessary,
    // since the sublevel will be centered around zero anyways.
    if (!world->OriginLocation.IsZero()) {
      world->SetNewWorldOrigin(FIntVector::ZeroValue);
    }
    return;
  }

  // We're either not in a sublevel, or OriginRebaseInsideSublevels is true.
  // Check whether a rebasing is necessary.
  const FIntVector& originLocation = world->OriginLocation;
  const FMinimalViewInfo& pov = this->WorldOriginCamera->ViewTarget.POV;
  const FVector& cameraLocation = pov.Location;
  bool distanceTooLarge = !cameraLocation.Equals(
      FVector::ZeroVector,
      this->MaximumWorldOriginDistanceFromCamera);
  if (distanceTooLarge) {
    // Camera has moved too far from the origin, move the origin,
    // but make sure that no component exceeds the maximum value
    // that can be represented as a 32bit signed integer.
    int32 newX = clampedAdd(cameraLocation.X, originLocation.X);
    int32 newY = clampedAdd(cameraLocation.Y, originLocation.Y);
    int32 newZ = clampedAdd(cameraLocation.Z, originLocation.Z);
    world->SetNewWorldOrigin(FIntVector(newX, newY, newZ));
  }
}

void ACesiumGeoreference::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);

#if WITH_EDITOR
  _showSubLevelLoadRadii();
  _handleViewportOriginEditing();
#endif

  this->_insideSublevel = _updateSublevelState();
  _performOriginRebasing();
}

/**
 * Useful Conversion Functions
 */

glm::dvec3 ACesiumGeoreference::TransformLongitudeLatitudeHeightToEcef(
    const glm::dvec3& longitudeLatitudeHeight) const {
  return _geoTransforms.TransformLongitudeLatitudeHeightToEcef(
      longitudeLatitudeHeight);
}

FVector ACesiumGeoreference::InaccurateTransformLongitudeLatitudeHeightToEcef(
    const FVector& longitudeLatitudeHeight) const {
  glm::dvec3 ecef = this->_geoTransforms.TransformLongitudeLatitudeHeightToEcef(
      VecMath::createVector3D(longitudeLatitudeHeight));
  return FVector(ecef.x, ecef.y, ecef.z);
}

glm::dvec3 ACesiumGeoreference::TransformEcefToLongitudeLatitudeHeight(
    const glm::dvec3& ecef) const {
  return _geoTransforms.TransformEcefToLongitudeLatitudeHeight(ecef);
}

FVector ACesiumGeoreference::InaccurateTransformEcefToLongitudeLatitudeHeight(
    const FVector& ecef) const {
  glm::dvec3 llh = this->_geoTransforms.TransformEcefToLongitudeLatitudeHeight(
      glm::dvec3(ecef.X, ecef.Y, ecef.Z));
  return FVector(llh.x, llh.y, llh.z);
}

glm::dvec3 ACesiumGeoreference::TransformLongitudeLatitudeHeightToUnreal(
    const glm::dvec3& longitudeLatitudeHeight) const {
  return this->_geoTransforms.TransformLongitudeLatitudeHeightToUnreal(
      CesiumActors::getWorldOrigin4D(this),
      longitudeLatitudeHeight);
}

FVector ACesiumGeoreference::InaccurateTransformLongitudeLatitudeHeightToUnreal(
    const FVector& longitudeLatitudeHeight) const {
  glm::dvec3 ue = this->_geoTransforms.TransformLongitudeLatitudeHeightToUnreal(
      CesiumActors::getWorldOrigin4D(this),
      VecMath::createVector3D(longitudeLatitudeHeight));
  return FVector(ue.x, ue.y, ue.z);
}

glm::dvec3 ACesiumGeoreference::TransformUnrealToLongitudeLatitudeHeight(
    const glm::dvec3& ue) const {
  return this->_geoTransforms.TransformUnrealToLongitudeLatitudeHeight(
      CesiumActors::getWorldOrigin4D(this),
      ue);
}

FVector ACesiumGeoreference::InaccurateTransformUnrealToLongitudeLatitudeHeight(
    const FVector& ue) const {
  glm::dvec3 llh =
      this->_geoTransforms.TransformUnrealToLongitudeLatitudeHeight(
          CesiumActors::getWorldOrigin4D(this),
          VecMath::createVector3D(ue));
  return FVector(llh.x, llh.y, llh.z);
}

glm::dvec3
ACesiumGeoreference::TransformEcefToUnreal(const glm::dvec3& ecef) const {
  return this->_geoTransforms.TransformEcefToUnreal(
      CesiumActors::getWorldOrigin4D(this),
      ecef);
}

FVector ACesiumGeoreference::InaccurateTransformEcefToUnreal(
    const FVector& ecef) const {
  glm::dvec3 ue = this->_geoTransforms.TransformEcefToUnreal(
      CesiumActors::getWorldOrigin4D(this),
      VecMath::createVector3D(ecef));
  return FVector(ue.x, ue.y, ue.z);
}

glm::dvec3
ACesiumGeoreference::TransformUnrealToEcef(const glm::dvec3& ue) const {
  return this->_geoTransforms.TransformUnrealToEcef(
      CesiumActors::getWorldOrigin4D(this),
      ue);
}

FVector
ACesiumGeoreference::InaccurateTransformUnrealToEcef(const FVector& ue) const {
  glm::dvec3 ecef = this->_geoTransforms.TransformUnrealToEcef(
      CesiumActors::getWorldOrigin4D(this),
      glm::dvec3(ue.X, ue.Y, ue.Z));
  return FVector(ecef.x, ecef.y, ecef.z);
}

glm::dquat ACesiumGeoreference::TransformRotatorUnrealToEastNorthUp(
    const glm::dquat& UeRotator,
    const glm::dvec3& UeLocation) const {
  return this->_geoTransforms.TransformRotatorUnrealToEastNorthUp(
      CesiumActors::getWorldOrigin4D(this),
      UeRotator,
      UeLocation);
}

FRotator ACesiumGeoreference::InaccurateTransformRotatorUnrealToEastNorthUp(
    const FRotator& UERotator,
    const FVector& ueLocation) const {
  glm::dquat q = TransformRotatorUnrealToEastNorthUp(
      VecMath::createQuaternion(UERotator.Quaternion()),
      VecMath::createVector3D(ueLocation));
  return VecMath::createRotator(q);
}

glm::dquat ACesiumGeoreference::TransformRotatorEastNorthUpToUnreal(
    const glm::dquat& EnuRotator,
    const glm::dvec3& UeLocation) const {
  return this->_geoTransforms.TransformRotatorEastNorthUpToUnreal(
      CesiumActors::getWorldOrigin4D(this),
      EnuRotator,
      UeLocation);
}

FRotator ACesiumGeoreference::InaccurateTransformRotatorEastNorthUpToUnreal(
    const FRotator& ENURotator,
    const FVector& ueLocation) const {
  glm::dquat q = TransformRotatorEastNorthUpToUnreal(
      VecMath::createQuaternion(ENURotator.Quaternion()),
      VecMath::createVector3D(ueLocation));
  return VecMath::createRotator(q);
}

glm::dmat3
ACesiumGeoreference::ComputeEastNorthUpToUnreal(const glm::dvec3& ue) const {
  return _geoTransforms.ComputeEastNorthUpToUnreal(
      CesiumActors::getWorldOrigin4D(this),
      ue);
}

FMatrix ACesiumGeoreference::InaccurateComputeEastNorthUpToUnreal(
    const FVector& ue) const {
  glm::dmat3 enuToUnreal = this->_geoTransforms.ComputeEastNorthUpToUnreal(
      CesiumActors::getWorldOrigin4D(this),
      glm::dvec3(ue.X, ue.Y, ue.Z));
  return VecMath::createMatrix(enuToUnreal);
}

glm::dmat3
ACesiumGeoreference::ComputeEastNorthUpToEcef(const glm::dvec3& ecef) const {
  return _geoTransforms.ComputeEastNorthUpToEcef(ecef);
}

FMatrix ACesiumGeoreference::InaccurateComputeEastNorthUpToEcef(
    const FVector& ecef) const {
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
