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
#include "WorldBrowserModule.h"
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

  this->_enableAndGeoreferenceCurrentSubLevel();
#endif
}

namespace {

template <typename TStringish>
FString longPackageNameToCesiumName(UWorld* pWorld, const TStringish& name) {
  FString levelName = FPackageName::GetShortName(name);
  levelName.RemoveFromStart(pWorld->StreamingLevelsPrefix);
  return levelName;
}

struct WorldCompositionLevelPair {
  FWorldCompositionTile* pTile;
  ULevelStreaming* pLevelStreaming;
};

WorldCompositionLevelPair findWorldCompositionLevel(
    UWorldComposition* pWorldComposition,
    const FName& packageName) {

  UWorldComposition::FTilesList& tiles = pWorldComposition->GetTilesList();
  const TArray<ULevelStreaming*>& levels = pWorldComposition->TilesStreaming;

  for (int32 i = 0; i < tiles.Num(); ++i) {
    FWorldCompositionTile& tile = tiles[i];

    if (tile.Info.Layer.DistanceStreamingEnabled) {
      // UE itself is managing distance-based streaming for this level, ignore
      // it.
      continue;
    }

    if (tile.PackageName == packageName) {
      assert(i < levels.Num());
      return {&tile, levels[i]};
    }
  }

  return {nullptr, nullptr};
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

  const UWorldComposition::FTilesList& allLevels =
      pWorld->WorldComposition->GetTilesList();

  TArray<int32> newSubLevels;
  TArray<int32> missingSubLevels;

  // const FLevelModelList& allLevels = pWorldModel->GetAllLevels();

  // Find new sub-levels that we don't know about on the Cesium side.
  for (int32 i = 0; i < allLevels.Num(); ++i) {
    const FWorldCompositionTile& level = allLevels[i];

    FString levelName = longPackageNameToCesiumName(pWorld, level.PackageName);

    // Check if the level is already known
    FCesiumSubLevel* pFound = this->CesiumSubLevels.FindByPredicate(
        [&levelName](FCesiumSubLevel& subLevel) {
          return levelName.Equals(subLevel.LevelName);
        });

    if (pFound) {
      pFound->CanBeEnabled = !level.Info.Layer.DistanceStreamingEnabled;
      if (!pFound->CanBeEnabled) {
        pFound->Enabled = false;
      }
    } else {
      newSubLevels.Add(i);
    }
  }

  // Find any Cesium sub-levels that don't exist anymore.
  for (int32 i = 0; i < this->CesiumSubLevels.Num(); ++i) {
    FCesiumSubLevel& cesiumLevel = this->CesiumSubLevels[i];

    const FWorldCompositionTile* pLevel = allLevels.FindByPredicate(
        [&cesiumLevel, pWorld](const FWorldCompositionTile& level) {
          return longPackageNameToCesiumName(pWorld, level.PackageName) ==
                 cesiumLevel.LevelName;
        });

    if (!pLevel) {
      missingSubLevels.Add(i);
    }
  }

  if (newSubLevels.Num() == 1 && missingSubLevels.Num() == 1) {
    // There is exactly one missing and one new, assume it's been renamed.
    const FWorldCompositionTile& level = allLevels[newSubLevels[0]];
    this->CesiumSubLevels[missingSubLevels[0]].LevelName =
        longPackageNameToCesiumName(pWorld, level.PackageName);
  } else if (newSubLevels.Num() > 0 || missingSubLevels.Num() > 0) {
    // Remove our record of the sub-levels that no longer exist.
    // Do this in reverse order so the indices don't get invalidated.
    for (int32 i = missingSubLevels.Num() - 1; i >= 0; --i) {
      this->CesiumSubLevels.RemoveAt(missingSubLevels[i]);
    }

    // Add new Cesium records for the new sub-levels
    for (int32 i = 0; i < newSubLevels.Num(); ++i) {
      const FWorldCompositionTile& level = allLevels[newSubLevels[i]];
      bool canBeEnabled = !level.Info.Layer.DistanceStreamingEnabled;
      this->CesiumSubLevels.Add(FCesiumSubLevel{
          longPackageNameToCesiumName(pWorld, level.PackageName),
          canBeEnabled,
          OriginLongitude,
          OriginLatitude,
          OriginHeight,
          1000.0,
          canBeEnabled});
    }
  }
}
#endif

bool ACesiumGeoreference::SwitchToLevel(int32 Index) {
  FCesiumSubLevel* pCurrentLevel = nullptr;

  if (Index >= 0 && Index < this->CesiumSubLevels.Num()) {
    pCurrentLevel = &this->CesiumSubLevels[Index];
  }

#if WITH_EDITOR
  // In the Editor, use the Editor API to make the selected level the current
  // one. When running in the Editor, GetStreamingLevels only includes levels
  // that are already loaded.
  if (GEditor && this->GetWorld() && !this->GetWorld()->IsGameWorld()) {
    return this->_switchToLevelInEditor(pCurrentLevel);
  }
#endif

  return this->_switchToLevelInGame(pCurrentLevel);
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

  PrimaryActorTick.TickGroup = TG_PostUpdateWork;

  UWorld* pWorld = this->GetWorld();
  if (!pWorld) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreference does not have a World in BeginPlay."));
    return;
  }

  if (!this->WorldOriginCamera) {
    // Find the first player's camera manager
    APlayerController* pPlayerController = pWorld->GetFirstPlayerController();
    if (pPlayerController) {
      this->WorldOriginCamera = pPlayerController->PlayerCameraManager;
    }

    if (!this->WorldOriginCamera) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "CesiumGeoreference could not find a FirstPlayerController or a corresponding PlayerCameraManager."));
    }
  }

  // initialize sublevels as unloaded
  for (ULevelStreaming* pLevel : pWorld->GetStreamingLevels()) {
    pLevel->SetShouldBeLoaded(false);
    pLevel->SetShouldBeVisible(false);
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

#if WITH_EDITOR
  // Subscribe to "browse a new world" events if we haven't already.
  // When we start browsing a new world, we'll subscribe to be notified
  // of new sub-levels in that world.
  if (!this->_onBrowseWorldSubscription.IsValid()) {
    FWorldBrowserModule* pWorldBrowserModule =
        static_cast<FWorldBrowserModule*>(
            FModuleManager::Get().GetModule("WorldBrowser"));
    if (pWorldBrowserModule) {
      this->_onBrowseWorldSubscription =
          pWorldBrowserModule->OnBrowseWorld.AddUObject(
              this,
              &ACesiumGeoreference::_onBrowseWorld);
    }
  }

  if (!this->_onEnginePreExitSubscription.IsValid()) {
    // On Editor close, Unreal Engine unhelpfully shuts down the WoldBrowser
    // module before it calls BeginDestroy on this Actor. And WorldBrowser
    // shutdown asserts that no one else is holding a reference to the world
    // model. So, we need to catch engine exit and detach ourselves.
    this->_onEnginePreExitSubscription =
        FCoreDelegates::OnEnginePreExit.AddUObject(
            this,
            &ACesiumGeoreference::_removeSubscriptions);
  }

  if (!this->_newCurrentLevelSubscription.IsValid()) {
    this->_newCurrentLevelSubscription =
        FEditorDelegates::NewCurrentLevel.AddUObject(
            this,
            &ACesiumGeoreference::_onNewCurrentLevel);
  }
#endif

  this->UpdateGeoreference();
}

void ACesiumGeoreference::BeginDestroy() {
#if WITH_EDITOR
  this->_removeSubscriptions();
#endif

  Super::BeginDestroy();
}

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
  if (this->CesiumSubLevels.Num() == 0) {
    // If we don't have any known sublevels, bail quickly to save ourselves a
    // little work.
    return false;
  }

  if (!IsValid(WorldOriginCamera)) {
    return false;
  }

  UWorld* pWorld = this->GetWorld();
  const FIntVector& originLocation = pWorld->OriginLocation;
  const FMinimalViewInfo& pov = this->WorldOriginCamera->ViewTarget.POV;
  const FVector& cameraLocation = pov.Location;

  glm::dvec4 cameraAbsolute = VecMath::add4D(cameraLocation, originLocation);

  glm::dvec3 cameraECEF =
      this->_geoTransforms
          .GetAbsoluteUnrealWorldToEllipsoidCenteredTransform() *
      cameraAbsolute;

  int32 activeLevel = -1;
  double closestLevelDistance = std::numeric_limits<double>::max();

  for (int32 i = 0; i < this->CesiumSubLevels.Num(); ++i) {
    const FCesiumSubLevel& level = this->CesiumSubLevels[i];
    if (!level.Enabled) {
      continue;
    }

    glm::dvec3 levelECEF =
        _geoTransforms.TransformLongitudeLatitudeHeightToEcef(glm::dvec3(
            level.LevelLongitude,
            level.LevelLatitude,
            level.LevelHeight));

    double levelDistance = glm::length(levelECEF - cameraECEF);
    if (levelDistance < level.LoadRadius &&
        levelDistance < closestLevelDistance) {
      activeLevel = i;
    }
  }

  // activeLevel may be -1, in which case all levels will be deactivated.
  return this->SwitchToLevel(activeLevel);
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
      center);
}

void ACesiumGeoreference::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);

  // A Georeference inside a sublevel should not do anything.
  if (!this->GetLevel()->IsPersistentLevel()) {
    return;
  }

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

  this->_insideSublevel = _updateSublevelState();
  _performOriginRebasing();
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

ULevelStreaming*
ACesiumGeoreference::_findLevelStreamingByName(const FString& name) {
  UWorld* pWorld = this->GetWorld();
  const TArray<ULevelStreaming*>& levels = pWorld->GetStreamingLevels();
  const FString& streamingLevelPrefix = pWorld->StreamingLevelsPrefix;

  ULevelStreaming* const* ppStreamedLevel =
      levels.FindByPredicate([&name, pWorld](ULevelStreaming* pStreamedLevel) {
        return longPackageNameToCesiumName(
                   pWorld,
                   pStreamedLevel->GetWorldAssetPackageName()) == name;
      });

  if (ppStreamedLevel == nullptr) {
    return nullptr;
  } else {
    return *ppStreamedLevel;
  }
}

FCesiumSubLevel* ACesiumGeoreference::_findCesiumSubLevelByName(
    const FName& packageName,
    bool createIfDoesNotExist) {
  FString cesiumName =
      longPackageNameToCesiumName(this->GetWorld(), packageName);

  FCesiumSubLevel* pCesiumLevel = this->CesiumSubLevels.FindByPredicate(
      [cesiumName](const FCesiumSubLevel& level) {
        return cesiumName == level.LevelName;
      });

  if (!pCesiumLevel && createIfDoesNotExist) {
    // No Cesium sub-level exists, so create it now.
    this->CesiumSubLevels.Add(FCesiumSubLevel{
        cesiumName,
        true,
        this->OriginLongitude,
        this->OriginLatitude,
        this->OriginHeight,
        1000.0,
        true});
    pCesiumLevel = &this->CesiumSubLevels.Last();
  }

  return pCesiumLevel;
}

#if WITH_EDITOR
void ACesiumGeoreference::_onBrowseWorld(UWorld* pWorld) {
  if (this->_levelsCollectionSubscription.IsValid()) {
    this->_pWorldModel->CollectionChanged.Remove(
        this->_levelsCollectionSubscription);
    this->_levelsCollectionSubscription.Reset();
    this->_pWorldModel.Reset();
  }

  if (pWorld) {
    FWorldBrowserModule& worldBrowserModule =
        FModuleManager::GetModuleChecked<FWorldBrowserModule>("WorldBrowser");
    this->_pWorldModel = worldBrowserModule.SharedWorldModel(pWorld);

    if (this->_pWorldModel) {
      this->_levelsCollectionSubscription =
          this->_pWorldModel->CollectionChanged.AddUObject(
              this,
              &ACesiumGeoreference::_updateCesiumSubLevels);
    }
  }
}

void ACesiumGeoreference::_removeSubscriptions() {
  this->_onBrowseWorld(nullptr);

  if (this->_onEnginePreExitSubscription.IsValid()) {
    FCoreDelegates::OnEnginePreExit.Remove(this->_onEnginePreExitSubscription);
    this->_onEnginePreExitSubscription.Reset();
  }

  if (this->_onBrowseWorldSubscription.IsValid()) {
    FWorldBrowserModule* pWorldBrowserModule =
        static_cast<FWorldBrowserModule*>(
            FModuleManager::Get().GetModule("WorldBrowser"));
    if (pWorldBrowserModule) {
      pWorldBrowserModule->OnBrowseWorld.Remove(
          this->_onBrowseWorldSubscription);
      this->_onBrowseWorldSubscription.Reset();
    }
  }

  if (this->_newCurrentLevelSubscription.IsValid()) {
    FEditorDelegates::NewCurrentLevel.Remove(
        this->_newCurrentLevelSubscription);
    this->_newCurrentLevelSubscription.Reset();
  }
}

bool ACesiumGeoreference::_switchToLevelInEditor(FCesiumSubLevel* pLevel) {
  UWorld* pWorld = this->GetWorld();

  FWorldBrowserModule& worldBrowserModule =
      FModuleManager::GetModuleChecked<FWorldBrowserModule>("WorldBrowser");
  TSharedPtr<FLevelCollectionModel> pWorldModel =
      worldBrowserModule.SharedWorldModel(pWorld);

  if (!pWorldModel) {
    return false;
  }

  const FLevelModelList& allLevels = pWorldModel->GetAllLevels();
  FLevelModelList levelsToHide = allLevels;

  // Don't hide persistent levels.
  levelsToHide.RemoveAll([](const TSharedPtr<FLevelModel>& pLevel) {
    return pLevel->IsPersistent();
  });

  bool result = false;

  if (pLevel) {
    // Find the one level we want active.
    const TSharedPtr<FLevelModel>* ppLevel = allLevels.FindByPredicate(
        [pLevel, pWorld](const TSharedPtr<FLevelModel>& pLevelModel) {
          return longPackageNameToCesiumName(
                     pWorld,
                     pLevelModel->GetLongPackageName()) == pLevel->LevelName;
        });

    if (ppLevel && *ppLevel) {
      levelsToHide.Remove(*ppLevel);

      (*ppLevel)->MakeLevelCurrent();

      this->SetGeoreferenceOrigin(glm::dvec3(
          pLevel->LevelLongitude,
          pLevel->LevelLatitude,
          pLevel->LevelHeight));

      result = true;
    }
  }

  // Unload all other levels.
  pWorldModel->HideLevels(levelsToHide);

  return result;
}

void ACesiumGeoreference::_onNewCurrentLevel() {
  UWorld* pWorld = this->GetWorld();
  if (!pWorld) {
    return;
  }

  UWorldComposition* pWorldComposition = pWorld->WorldComposition;
  if (!pWorldComposition) {
    return;
  }

  ULevel* pCurrent = pWorld->GetCurrentLevel();
  UPackage* pLevelPackage = pCurrent->GetOutermost();

  // Find the world composition details for the new level.
  WorldCompositionLevelPair worldCompositionLevelPair =
      findWorldCompositionLevel(pWorldComposition, pLevelPackage->GetFName());

  FWorldCompositionTile* pTile = worldCompositionLevelPair.pTile;
  ULevelStreaming* pLevelStreaming = worldCompositionLevelPair.pLevelStreaming;

  if (!pTile || !pLevelStreaming) {
    // The new level doesn't appear to participate in world composition, so
    // ignore it.
    return;
  }

  // Find the corresponding FCesiumSubLevel, creating it if necessary.
  FCesiumSubLevel* pCesiumLevel =
      this->_findCesiumSubLevelByName(pLevelPackage->GetFName(), true);
  if (!pCesiumLevel->Enabled) {
    return;
  }

  // Hide all other levels.
  // I initially thought we could call handy methods like SetShouldBeVisible
  // here, but I was be wrong. That works ok in a game, but in the Editor
  // there's a much more elaborate dance required. Rather than try to figure out
  // all the steps, let's just ask the Editor to do it for us.
  FWorldBrowserModule& worldBrowserModule =
      FModuleManager::GetModuleChecked<FWorldBrowserModule>("WorldBrowser");
  TSharedPtr<FLevelCollectionModel> pWorldModel =
      worldBrowserModule.SharedWorldModel(pWorld);
  if (!pWorldModel) {
    return;
  }

  // Build a list of levels to hide, starting with _all_ the levels.
  const FLevelModelList& allLevels = pWorldModel->GetAllLevels();
  FLevelModelList levelsToHide = allLevels;

  // Remove levels from the list that we don't want to hide for various reasons.
  levelsToHide.RemoveAll(
      [pCurrent, pWorldComposition](const TSharedPtr<FLevelModel>& pLevel) {
        // Remove persistent levels.
        if (pLevel->IsPersistent()) {
          return true;
        }

        // Remove the now-current level.
        if (pLevel->GetLevelObject() == pCurrent) {
          return true;
        }

        WorldCompositionLevelPair compositionLevel = findWorldCompositionLevel(
            pWorldComposition,
            pLevel->GetLongPackageName());

        // Remove levels that are not part of the world composition.
        if (!compositionLevel.pLevelStreaming || !compositionLevel.pTile) {
          return true;
        }

        // Remove levels that Unreal Engine is handling distance-based streaming
        // for. Hiding such a level that UE thinks should be shown would cause
        // the level to toggle on and off continually.
        if (compositionLevel.pTile->Info.Layer.DistanceStreamingEnabled) {
          return true;
        }

        return false;
      });

  pWorldModel->HideLevels(levelsToHide);

  // Set the georeference origin for the new level
  this->SetGeoreferenceOrigin(glm::dvec3(
      pCesiumLevel->LevelLongitude,
      pCesiumLevel->LevelLatitude,
      pCesiumLevel->LevelHeight));
}

void ACesiumGeoreference::_enableAndGeoreferenceCurrentSubLevel() {
  // If a sub-level is active, enable it and also update the sub-level's
  // location.
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
  }

  pLevel->Enabled = pLevel->CanBeEnabled;
}

#endif

bool ACesiumGeoreference::_switchToLevelInGame(FCesiumSubLevel* pLevel) {
  UWorld* pWorld = this->GetWorld();
  const TArray<ULevelStreaming*>& levels = pWorld->GetStreamingLevels();

  ULevelStreaming* pStreamedLevel = nullptr;

  if (pLevel) {
    // Find the streaming level with this name.
    pStreamedLevel = this->_findLevelStreamingByName(pLevel->LevelName);
    if (!pStreamedLevel) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("No streaming level found with name %s"),
          *pLevel->LevelName);
    }
  }

  // Deactivate all other streaming levels controlled by Cesium
  for (const FCesiumSubLevel& subLevel : this->CesiumSubLevels) {
    if (&subLevel == pLevel) {
      continue;
    }

    ULevelStreaming* pOtherLevel =
        this->_findLevelStreamingByName(subLevel.LevelName);

    if (pOtherLevel->ShouldBeVisible() || pOtherLevel->ShouldBeLoaded()) {
      pOtherLevel->SetShouldBeLoaded(false);
      pOtherLevel->SetShouldBeVisible(false);
    }
  }

  // Activate the new streaming level if it's not already active.
  if (pStreamedLevel && (!pStreamedLevel->ShouldBeVisible() ||
                         !pStreamedLevel->ShouldBeLoaded())) {
    this->_setGeoreferenceOrigin(
        pLevel->LevelLongitude,
        pLevel->LevelLatitude,
        pLevel->LevelHeight);

    pStreamedLevel->SetShouldBeLoaded(true);
    pStreamedLevel->SetShouldBeVisible(true);
  }

  return pStreamedLevel != nullptr;
}
