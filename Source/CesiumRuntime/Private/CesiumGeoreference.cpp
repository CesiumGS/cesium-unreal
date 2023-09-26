// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGeoreference.h"
#include "Camera/PlayerCameraManager.h"
#include "CesiumActors.h"
#include "CesiumCommon.h"
#include "CesiumGeospatial/Cartographic.h"
#include "CesiumRuntime.h"
#include "CesiumSubLevelComponent.h"
#include "CesiumSubLevelSwitcherComponent.h"
#include "CesiumTransforms.h"
#include "CesiumUtility/Math.h"
#include "CesiumWgs84Ellipsoid.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "Engine/WorldComposition.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GeoTransforms.h"
#include "LevelInstance/LevelInstanceActor.h"
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

using namespace CesiumGeospatial;

namespace {

LocalHorizontalCoordinateSystem
createCoordinateSystem(const FVector& center, double scale) {
  return LocalHorizontalCoordinateSystem(
      VecMath::createVector3D(center),
      LocalDirection::East,
      LocalDirection::South,
      LocalDirection::Up,
      1.0 / scale,
      Ellipsoid::WGS84);
}

} // namespace

/*static*/ const double ACesiumGeoreference::kMinimumScale = 1.0e-6;

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
    if (actor->GetLevel() == world->PersistentLevel &&
        actor->ActorHasTag(DEFAULT_GEOREFERENCE_TAG)) {
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
    spawnParameters.OverrideLevel = world->PersistentLevel;
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

FVector ACesiumGeoreference::GetOriginLongitudeLatitudeHeight() const {
  return FVector(OriginLongitude, OriginLatitude, OriginHeight);
}

void ACesiumGeoreference::SetOriginLongitudeLatitudeHeight(
    const FVector& targetLongitudeLatitudeHeight) {
  this->OriginLongitude = targetLongitudeLatitudeHeight.X;
  this->OriginLatitude = targetLongitudeLatitudeHeight.Y;
  this->OriginHeight = targetLongitudeLatitudeHeight.Z;
  this->UpdateGeoreference();
}

FVector ACesiumGeoreference::GetOriginEarthCenteredEarthFixed() const {
  return UCesiumWgs84Ellipsoid::
      LongitudeLatitudeHeightToEarthCenteredEarthFixed(
          this->GetOriginLongitudeLatitudeHeight());
}

void ACesiumGeoreference::SetOriginEarthCenteredEarthFixed(
    const FVector& TargetEarthCenteredEarthFixed) {
  this->SetOriginLongitudeLatitudeHeight(
      UCesiumWgs84Ellipsoid::EarthCenteredEarthFixedToLongitudeLatitudeHeight(
          TargetEarthCenteredEarthFixed));
}

EOriginPlacement ACesiumGeoreference::GetOriginPlacement() const {
  return this->OriginPlacement;
}

void ACesiumGeoreference::SetOriginPlacement(EOriginPlacement NewValue) {
  this->OriginPlacement = NewValue;
  this->UpdateGeoreference();
}

double ACesiumGeoreference::GetOriginLatitude() const {
  return this->OriginLatitude;
}

void ACesiumGeoreference::SetOriginLatitude(double NewValue) {
  this->OriginLatitude = NewValue;
  this->UpdateGeoreference();
}

double ACesiumGeoreference::GetOriginLongitude() const {
  return this->OriginLongitude;
}

void ACesiumGeoreference::SetOriginLongitude(double NewValue) {
  this->OriginLongitude = NewValue;
  this->UpdateGeoreference();
}

double ACesiumGeoreference::GetOriginHeight() const {
  return this->OriginHeight;
}

void ACesiumGeoreference::SetOriginHeight(double NewValue) {
  this->OriginHeight = NewValue;
  this->UpdateGeoreference();
}

double ACesiumGeoreference::GetScale() const { return this->Scale; }

void ACesiumGeoreference::SetScale(double NewValue) {
  if (NewValue < ACesiumGeoreference::kMinimumScale) {
    this->Scale = ACesiumGeoreference::kMinimumScale;
  } else {
    this->Scale = NewValue;
  }
  this->UpdateGeoreference();
}

APlayerCameraManager* ACesiumGeoreference::GetSubLevelCamera() const {
  return this->SubLevelCamera;
}

void ACesiumGeoreference::SetSubLevelCamera(APlayerCameraManager* NewValue) {
  this->SubLevelCamera = NewValue;
}

#if WITH_EDITOR
bool ACesiumGeoreference::GetShowLoadRadii() const {
  return this->ShowLoadRadii;
}

void ACesiumGeoreference::SetShowLoadRadii(bool NewValue) {
  this->ShowLoadRadii = NewValue;
}
#endif // WITH_EDITOR

FVector ACesiumGeoreference::TransformLongitudeLatitudeHeightPositionToUnreal(
    const FVector& LongitudeLatitudeHeight) const {
  return this->TransformEarthCenteredEarthFixedPositionToUnreal(
      UCesiumWgs84Ellipsoid::LongitudeLatitudeHeightToEarthCenteredEarthFixed(
          LongitudeLatitudeHeight));
}

FVector ACesiumGeoreference::TransformUnrealPositionToLongitudeLatitudeHeight(
    const FVector& UnrealPosition) const {
  return UCesiumWgs84Ellipsoid::
      EarthCenteredEarthFixedToLongitudeLatitudeHeight(
          this->TransformUnrealPositionToEarthCenteredEarthFixed(
              UnrealPosition));
}

FVector ACesiumGeoreference::TransformEarthCenteredEarthFixedPositionToUnreal(
    const FVector& EarthCenteredEarthFixedPosition) const {
  return VecMath::createVector(this->_coordinateSystem.ecefPositionToLocal(
      VecMath::createVector3D(EarthCenteredEarthFixedPosition)));
}

FVector ACesiumGeoreference::TransformUnrealPositionToEarthCenteredEarthFixed(
    const FVector& UnrealPosition) const {
  return VecMath::createVector(this->_coordinateSystem.localPositionToEcef(
      VecMath::createVector3D(UnrealPosition)));
}

FVector ACesiumGeoreference::TransformEarthCenteredEarthFixedDirectionToUnreal(
    const FVector& EarthCenteredEarthFixedDirection) const {
  return VecMath::createVector(this->_coordinateSystem.ecefDirectionToLocal(
      VecMath::createVector3D(EarthCenteredEarthFixedDirection)));
}

FVector ACesiumGeoreference::TransformUnrealDirectionToEarthCenteredEarthFixed(
    const FVector& UnrealDirection) const {
  return VecMath::createVector(this->_coordinateSystem.localDirectionToEcef(
      VecMath::createVector3D(UnrealDirection)));
}

FRotator ACesiumGeoreference::TransformUnrealRotatorToEastSouthUp(
    const FRotator& UnrealRotator,
    const FVector& UnrealLocation) const {
  FMatrix unrealToEsu =
      this->ComputeEastSouthUpToUnrealTransformation(UnrealLocation).Inverse();
  return FRotator(unrealToEsu.ToQuat() * UnrealRotator.Quaternion());
}

FRotator ACesiumGeoreference::TransformEastSouthUpRotatorToUnreal(
    const FRotator& EastSouthUpRotator,
    const FVector& UnrealLocation) const {
  FMatrix esuToUnreal =
      this->ComputeEastSouthUpToUnrealTransformation(UnrealLocation);
  return FRotator(esuToUnreal.ToQuat() * EastSouthUpRotator.Quaternion());
}

FMatrix
ACesiumGeoreference::ComputeUnrealToEarthCenteredEarthFixedTransformation()
    const {
  return VecMath::createMatrix(
      this->_coordinateSystem.getLocalToEcefTransformation());
}

FMatrix
ACesiumGeoreference::ComputeEarthCenteredEarthFixedToUnrealTransformation()
    const {
  return VecMath::createMatrix(
      this->_coordinateSystem.getEcefToLocalTransformation());
}

FMatrix ACesiumGeoreference::ComputeEastSouthUpToUnrealTransformation(
    const FVector& UnrealLocation) const {
  FVector ecef =
      this->TransformUnrealPositionToEarthCenteredEarthFixed(UnrealLocation);
  LocalHorizontalCoordinateSystem newLocal =
      createCoordinateSystem(ecef, this->GetScale());
  return VecMath::createMatrix(
      newLocal.computeTransformationToAnotherLocal(this->_coordinateSystem));
}

FMatrix ACesiumGeoreference::ComputeUnrealToEastSouthUpTransformation(
    const FVector& UnrealLocation) const {
  return this->ComputeEastSouthUpToUnrealTransformation(UnrealLocation)
      .Inverse();
}

#if WITH_EDITOR
void ACesiumGeoreference::PlaceGeoreferenceOriginHere() {
  // If this is PIE mode, ignore
  UWorld* pWorld = this->GetWorld();
  if (!GEditor || pWorld->IsGameWorld()) {
    return;
  }

  this->Modify();

  FViewport* pViewport = GEditor->GetActiveViewport();
  FViewportClient* pViewportClient = pViewport->GetClient();
  FEditorViewportClient* pEditorViewportClient =
      static_cast<FEditorViewportClient*>(pViewportClient);

  FRotator newViewRotation = this->TransformUnrealRotatorToEastSouthUp(
      pEditorViewportClient->GetViewRotation(),
      pEditorViewportClient->GetViewLocation());

  // camera local space to ECEF
  FVector cameraEcefPosition =
      this->TransformUnrealPositionToEarthCenteredEarthFixed(
          pEditorViewportClient->GetViewLocation());

  // Long/Lat/Height camera location, in degrees/meters (also our new target
  // georeference origin) When the location is too close to the center of the
  // earth, the result will be (0,0,0)
  this->SetOriginEarthCenteredEarthFixed(cameraEcefPosition);

  // TODO: check for degeneracy ?
  FVector cameraFront = newViewRotation.RotateVector(FVector::XAxisVector);
  FVector cameraRight =
      FVector::CrossProduct(FVector::ZAxisVector, cameraFront).GetSafeNormal();
  FVector cameraUp =
      FVector::CrossProduct(cameraFront, cameraRight).GetSafeNormal();

  pEditorViewportClient->SetViewRotation(
      FMatrix(cameraFront, cameraRight, cameraUp, FVector::ZeroVector)
          .Rotator());
  pEditorViewportClient->SetViewLocation(FVector::ZeroVector);
}

void ACesiumGeoreference::_showSubLevelLoadRadii() const {
  UWorld* world = this->GetWorld();
  if (world->IsGameWorld()) {
    return;
  }
  if (!this->ShowLoadRadii) {
    return;
  }

  for (const auto& pLevelWeak :
       this->SubLevelSwitcher->GetRegisteredSubLevels()) {
    ALevelInstance* pLevel = pLevelWeak.Get();
    if (!IsValid(pLevel))
      continue;

    UCesiumSubLevelComponent* pComponent =
        pLevel->FindComponentByClass<UCesiumSubLevelComponent>();

    FVector center =
        this->TransformLongitudeLatitudeHeightPositionToUnreal(FVector(
            pComponent->GetOriginLongitude(),
            pComponent->GetOriginLatitude(),
            pComponent->GetOriginHeight()));
    center = GetActorTransform().TransformPosition(center);

    DrawDebugSphere(
        world,
        center,
        100.0 * pComponent->GetLoadRadius() * GetActorScale3D().GetMax(),
        100,
        FColor::Blue);
  }
}
#endif // WITH_EDITOR

bool ACesiumGeoreference::ShouldTickIfViewportsOnly() const { return true; }

void ACesiumGeoreference::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);

#if WITH_EDITOR
  _showSubLevelLoadRadii();
#endif

  if (this->_shouldManageSubLevels()) {
    _updateSublevelState();
  }
}

void ACesiumGeoreference::Serialize(FArchive& Ar) {
  Super::Serialize(Ar);

  // Recompute derived values on load.
  if (Ar.IsLoading()) {
    this->_updateCoordinateSystem();
  }
}

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

/** In case the CesiumGeoreference gets spawned at run time, instead of design
 *  time, ensure that frames are updated */
void ACesiumGeoreference::OnConstruction(const FTransform& Transform) {
  Super::OnConstruction(Transform);

  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called OnConstruction on actor %s"),
      *this->GetName());

  this->UpdateGeoreference();
}

void ACesiumGeoreference::PostLoad() {
  Super::PostLoad();

#if WITH_EDITOR
  if (GEditor && IsValid(this->GetWorld()) &&
      IsValid(this->GetWorld()->WorldComposition) &&
      !this->GetWorld()->IsGameWorld()) {
    this->_createSubLevelsFromWorldComposition();
  }
#endif // WITH_EDITOR
}

#if WITH_EDITOR
void ACesiumGeoreference::PostEditChangeProperty(FPropertyChangedEvent& event) {
  Super::PostEditChangeProperty(event);

  if (!event.Property) {
    return;
  }

  FName propertyName = event.Property->GetFName();

  CESIUM_POST_EDIT_CHANGE(propertyName, ACesiumGeoreference, OriginPlacement);
  CESIUM_POST_EDIT_CHANGE(propertyName, ACesiumGeoreference, OriginLongitude);
  CESIUM_POST_EDIT_CHANGE(propertyName, ACesiumGeoreference, OriginLatitude);
  CESIUM_POST_EDIT_CHANGE(propertyName, ACesiumGeoreference, OriginHeight);
  CESIUM_POST_EDIT_CHANGE(propertyName, ACesiumGeoreference, Scale);
}

namespace {

template <typename TStringish>
FString longPackageNameToCesiumName(UWorld* pWorld, const TStringish& name) {
  FString levelName = FPackageName::GetShortName(name);
  levelName.RemoveFromStart(pWorld->StreamingLevelsPrefix);
  return levelName;
}

} // namespace

PRAGMA_DISABLE_DEPRECATION_WARNINGS
void ACesiumGeoreference::_createSubLevelsFromWorldComposition() {
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

  // Convert old-style sub-levels (using World Composition) to new style
  // sub-levels (level instances)
  UWorldComposition::FTilesList& allLevels =
      pWorld->WorldComposition->GetTilesList();

  ALevelInstance* pActiveSubLevel = nullptr;

  for (const FWorldCompositionTile& level : allLevels) {
    FString levelName = longPackageNameToCesiumName(pWorld, level.PackageName);

    // Check if the level is already known
    FCesiumSubLevel* pFound = this->CesiumSubLevels_DEPRECATED.FindByPredicate(
        [&levelName](FCesiumSubLevel& subLevel) {
          return levelName.Equals(subLevel.LevelName);
        });

    // A sub-level that can't be enabled is being controlled by Unreal Engine,
    // based on its own distance-based system. Ignore it.
    if (pFound == nullptr || !pFound->CanBeEnabled)
      continue;

    FActorSpawnParameters spawnParameters{};
    spawnParameters.Name = FName(pFound->LevelName);
    spawnParameters.ObjectFlags = RF_Transactional;

    ALevelInstance* pLevelInstance = pWorld->SpawnActor<ALevelInstance>(
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        spawnParameters);
    pLevelInstance->SetIsSpatiallyLoaded(false);
    pLevelInstance->DesiredRuntimeBehavior =
        ELevelInstanceRuntimeBehavior::LevelStreaming;
    pLevelInstance->SetActorLabel(pFound->LevelName);

    FString levelPath = level.PackageName.ToString() + "." +
                        FPackageName::GetShortName(level.PackageName);
    TSoftObjectPtr<UWorld> asset{FSoftObjectPath(levelPath)};
    pLevelInstance->SetWorldAsset(asset);

    // Initially mark all sub-levels hidden in the Editor.
    pLevelInstance->SetIsTemporarilyHiddenInEditor(true);

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

    // But if the georeference origin is close to this sub-level's origin, make
    // this the active sub-level.
    if (FMath::IsNearlyEqual(
            this->OriginLongitude,
            pFound->LevelLongitude,
            1e-8) &&
        FMath::IsNearlyEqual(
            this->OriginLatitude,
            pFound->LevelLatitude,
            1e-8) &&
        FMath::IsNearlyEqual(this->OriginHeight, pFound->LevelHeight, 1e-3)) {
      pActiveSubLevel = pLevelInstance;
    }

    pLevelInstance->LoadLevelInstance();
  }

  this->SubLevelSwitcher->SetTarget(pActiveSubLevel);

  this->CesiumSubLevels_DEPRECATED.Empty();

  UE_LOG(
      LogCesium,
      Warning,
      TEXT(
          "Cesium sub-levels based on World Composition have been converted to Level Instances. Save the level to keep these changes. We recommend disabling World Composition in the World Settings, as it is now obsolete."));
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#endif // WITH_EDITOR

FVector ACesiumGeoreference::TransformLongitudeLatitudeHeightToEcef(
    const FVector& longitudeLatitudeHeight) const {
  return UCesiumWgs84Ellipsoid::
      LongitudeLatitudeHeightToEarthCenteredEarthFixed(longitudeLatitudeHeight);
}

FVector ACesiumGeoreference::TransformEcefToLongitudeLatitudeHeight(
    const FVector& ecef) const {
  return UCesiumWgs84Ellipsoid::
      EarthCenteredEarthFixedToLongitudeLatitudeHeight(ecef);
}

FMatrix
ACesiumGeoreference::ComputeEastNorthUpToEcef(const FVector& ecef) const {
  return UCesiumWgs84Ellipsoid::EastNorthUpToEarthCenteredEarthFixed(ecef);
}

ACesiumGeoreference::ACesiumGeoreference() : AActor() {
  PrimaryActorTick.bCanEverTick = true;

  this->Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
  this->RootComponent = this->Root;

#if WITH_EDITOR
  this->SetIsSpatiallyLoaded(false);
#endif
  this->SubLevelSwitcher =
      CreateDefaultSubobject<UCesiumSubLevelSwitcherComponent>(
          "SubLevelSwitcher");
}

void ACesiumGeoreference::UpdateGeoreference() {
  this->_updateCoordinateSystem();

  // If we're in a sub-level, update its origin as well.
  UCesiumSubLevelSwitcherComponent* pSwitcher = this->SubLevelSwitcher;
  if (IsValid(pSwitcher) && pSwitcher->GetTarget() != nullptr) {
    if (pSwitcher->GetTarget() == pSwitcher->GetCurrent() ||
        pSwitcher->GetCurrent() == nullptr) {
      ALevelInstance* pTarget = pSwitcher->GetTarget();
      UCesiumSubLevelComponent* pComponent =
          pTarget->FindComponentByClass<UCesiumSubLevelComponent>();
      if (IsValid(pComponent)) {
        pComponent->SetOriginLongitudeLatitudeHeight(FVector(
            this->OriginLongitude,
            this->OriginLatitude,
            this->OriginHeight));
      }
    }
  }

  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Broadcasting OnGeoreferenceUpdated for Georeference %s"),
      *this->GetFullName());

  OnGeoreferenceUpdated.Broadcast();
}

GeoTransforms ACesiumGeoreference::GetGeoTransforms() const noexcept {
  // Because GeoTransforms is deprecated, we only lazily update it.
  return GeoTransforms(
      Ellipsoid::WGS84,
      glm::dvec3(this->_coordinateSystem.getLocalToEcefTransformation()[3]),
      this->GetScale() / 100.0);
}

FName ACesiumGeoreference::DEFAULT_GEOREFERENCE_TAG =
    FName("DEFAULT_GEOREFERENCE");

bool ACesiumGeoreference::_updateSublevelState() {
  const TArray<TWeakObjectPtr<ALevelInstance>>& sublevels =
      this->SubLevelSwitcher->GetRegisteredSubLevels();

  if (sublevels.Num() == 0) {
    // If we don't have any known sub-levels, bail quickly to save ourselves a
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

  // Transform camera from World to Georeference local frame
  FVector cameraRelativeToGeoreference =
      this->GetActorTransform().TransformPosition(cameraLocation);

  // Transform camera from Georeference local frame to ECEF
  FVector cameraEcef = this->TransformUnrealPositionToEarthCenteredEarthFixed(
      cameraRelativeToGeoreference);

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

    FVector levelEcef =
        UCesiumWgs84Ellipsoid::LongitudeLatitudeHeightToEarthCenteredEarthFixed(
            FVector(
                pComponent->GetOriginLongitude(),
                pComponent->GetOriginLatitude(),
                pComponent->GetOriginHeight()));

    double levelDistance = FVector::Distance(levelEcef, cameraEcef);
    if (levelDistance < pComponent->GetLoadRadius() &&
        levelDistance < closestLevelDistance) {
      pClosestActiveLevel = pCurrent;
      closestLevelDistance = levelDistance;
    }
  }

  this->SubLevelSwitcher->SetTarget(pClosestActiveLevel);

  return pClosestActiveLevel != nullptr;
}

void ACesiumGeoreference::_updateCoordinateSystem() {
  if (this->OriginPlacement == EOriginPlacement::CartographicOrigin) {
    FVector origin = this->GetOriginLongitudeLatitudeHeight();
    this->_coordinateSystem = createCoordinateSystem(
        this->GetOriginEarthCenteredEarthFixed(),
        this->GetScale());
  } else {
    // In True Origin mode, we want a coordinate system that:
    // 1. Is at the origin,
    // 2. Inverts Y to create a left-handed coordinate system, and
    // 3. Uses the georeference's scale
    double scale = 1.0 / this->GetScale();
    glm::dmat4 localToEcef(
        glm::dvec4(scale, 0.0, 0.0, 0.0),
        glm::dvec4(0.0, -scale, 0.0, 0.0),
        glm::dvec4(0.0, 0.0, scale, 0.0),
        glm::dvec4(0.0, 0.0, 0.0, 1.0));
    this->_coordinateSystem = LocalHorizontalCoordinateSystem(localToEcef);
  }
}

bool ACesiumGeoreference::_shouldManageSubLevels() const {
  // Only a Georeference in the PersistentLevel should manage sub-levels.
  return this->GetLevel()->IsPersistentLevel();
}
