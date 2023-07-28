// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGeoreference.h"
#include "Camera/PlayerCameraManager.h"
#include "CesiumActors.h"
#include "CesiumCommon.h"
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
  this->_setGeoreferenceOrigin(
      targetLongitudeLatitudeHeight.X,
      targetLongitudeLatitudeHeight.Y,
      targetLongitudeLatitudeHeight.Z);
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
  if (NewValue < 1e-6) {
    this->Scale = 1e-6;
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

bool ACesiumGeoreference::GetShowLoadRadii() const {
  return this->ShowLoadRadii;
}

void ACesiumGeoreference::SetShowLoadRadii(bool NewValue) {
  this->ShowLoadRadii = NewValue;
}

FVector ACesiumGeoreference::TransformLongitudeLatitudeHeightPositionToUnreal(
    const FVector& LongitudeLatitudeHeight) const {
  glm::dvec3 ue = this->_geoTransforms.TransformLongitudeLatitudeHeightToUnreal(
      glm::dvec3(CesiumActors::getWorldOrigin4D(this)),
      VecMath::createVector3D(LongitudeLatitudeHeight));
  return FVector(ue.x, ue.y, ue.z);
}

FVector ACesiumGeoreference::TransformUnrealPositionToLongitudeLatitudeHeight(
    const FVector& UnrealPosition) const {
  glm::dvec3 llh =
      this->_geoTransforms.TransformUnrealToLongitudeLatitudeHeight(
          glm::dvec3(CesiumActors::getWorldOrigin4D(this)),
          VecMath::createVector3D(UnrealPosition));
  return FVector(llh.x, llh.y, llh.z);
}

FVector ACesiumGeoreference::TransformEarthCenteredEarthFixedPositionToUnreal(
    const FVector& EarthCenteredEarthFixedPosition) const {
  glm::dvec3 ue = this->_geoTransforms.TransformEcefToUnreal(
      glm::dvec3(CesiumActors::getWorldOrigin4D(this)),
      VecMath::createVector3D(EarthCenteredEarthFixedPosition));
  return FVector(ue.x, ue.y, ue.z);
}

FVector ACesiumGeoreference::TransformUnrealPositionToEarthCenteredEarthFixed(
    const FVector& UnrealPosition) const {
  glm::dvec3 ecef = this->_geoTransforms.TransformUnrealToEcef(
      glm::dvec3(CesiumActors::getWorldOrigin4D(this)),
      glm::dvec3(UnrealPosition.X, UnrealPosition.Y, UnrealPosition.Z));
  return FVector(ecef.x, ecef.y, ecef.z);
}

FVector ACesiumGeoreference::TransformEarthCenteredEarthFixedDirectionToUnreal(
    const FVector& EarthCenteredEarthFixedDirection) const {
  return this->_geoTransforms.GetEllipsoidCenteredToAbsoluteUnrealWorldMatrix()
      .TransformVector(EarthCenteredEarthFixedDirection);
}

FVector ACesiumGeoreference::TransformUnrealDirectionToEarthCenteredEarthFixed(
    const FVector& UnrealDirection) const {
  return this->_geoTransforms.GetAbsoluteUnrealWorldToEllipsoidCenteredMatrix()
      .TransformVector(UnrealDirection);
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

const FMatrix&
ACesiumGeoreference::ComputeUnrealToEarthCenteredEarthFixedTransformation()
    const {
  return this->_geoTransforms.GetAbsoluteUnrealWorldToEllipsoidCenteredMatrix();
}

const FMatrix&
ACesiumGeoreference::ComputeEarthCenteredEarthFixedToUnrealTransformation()
    const {
  return this->_geoTransforms.GetEllipsoidCenteredToAbsoluteUnrealWorldMatrix();
}

FMatrix ACesiumGeoreference::ComputeEastSouthUpToUnrealTransformation(
    const FVector& UnrealLocation) const {
  glm::dmat4 esuToUe = this->_geoTransforms.ComputeEastSouthUpToUnreal(
      glm::dvec3(CesiumActors::getWorldOrigin4D(this)),
      glm::dvec3(UnrealLocation.X, UnrealLocation.Y, UnrealLocation.Z));
  return VecMath::createMatrix(esuToUe);
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
}

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
    this->_updateGeoTransforms();
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
  glm::dmat3 enuToEcef = this->_geoTransforms.ComputeEastNorthUpToEcef(
      glm::dvec3(ecef.X, ecef.Y, ecef.Z));
  return VecMath::createMatrix(enuToEcef);
}

ACesiumGeoreference::ACesiumGeoreference() : AActor(), _geoTransforms() {
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
  this->_updateGeoTransforms();

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

FName ACesiumGeoreference::DEFAULT_GEOREFERENCE_TAG =
    FName("DEFAULT_GEOREFERENCE");

void ACesiumGeoreference::_setGeoreferenceOrigin(
    double targetLongitude,
    double targetLatitude,
    double targetHeight) {
  this->OriginLongitude = targetLongitude;
  this->OriginLatitude = targetLatitude;
  this->OriginHeight = targetHeight;

  this->UpdateGeoreference();
}

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

  glm::dvec4 cameraAbsolute = VecMath::add4D(cameraLocation, originLocation);

  glm::dmat4 ueGeoreferenceToUeWorld =
      VecMath::createMatrix4D(this->GetActorTransform().ToMatrixWithScale());

  const glm::dmat4& cesiumGeoreferenceToUeGeoreference =
      this->_geoTransforms.GetEllipsoidCenteredToAbsoluteUnrealWorldTransform();
  glm::dmat4 unrealWorldToCesiumGeoreference = glm::affineInverse(
      ueGeoreferenceToUeWorld * cesiumGeoreferenceToUeGeoreference);
  glm::dvec3 cameraECEF =
      glm::dvec3(unrealWorldToCesiumGeoreference * cameraAbsolute);

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
      CesiumGeospatial::Ellipsoid::WGS84,
      center,
      this->Scale / 100.0);
}

bool ACesiumGeoreference::_shouldManageSubLevels() const {
  // Only a Georeference in the PersistentLevel should manage sub-levels.
  return this->GetLevel()->IsPersistentLevel();
}
