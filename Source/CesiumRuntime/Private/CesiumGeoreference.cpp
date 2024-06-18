// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGeoreference.h"
#include "Camera/PlayerCameraManager.h"
#include "Cesium3DTileset.h"
#include "CesiumActors.h"
#include "CesiumCommon.h"
#include "CesiumCustomVersion.h"
#include "CesiumGeospatial/Cartographic.h"
#include "CesiumOriginShiftComponent.h"
#include "CesiumRuntime.h"
#include "CesiumSubLevelComponent.h"
#include "CesiumSubLevelSwitcherComponent.h"
#include "CesiumTransforms.h"
#include "CesiumUtility/Math.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "Engine/WorldComposition.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GeoTransforms.h"
#include "Kismet/GameplayStatics.h"
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
#include "EditorLevelUtils.h"
#include "EditorViewportClient.h"
#include "GameFramework/Pawn.h"
#include "LevelInstance/LevelInstanceEditorLevelStreaming.h"
#include "Slate/SceneViewport.h"
#endif

using namespace CesiumGeospatial;

namespace {

ACesiumGeoreference* FindGeoreferenceAncestor(AActor* Actor) {
  AActor* Current = Actor;

  while (IsValid(Current)) {
    ACesiumGeoreference* Georeference = Cast<ACesiumGeoreference>(Current);
    if (IsValid(Georeference)) {
      return Georeference;
    }
    Current = Current->GetAttachParentActor();
  }

  return nullptr;
}

ACesiumGeoreference*
FindGeoreferenceWithTag(const UObject* WorldContextObject, const FName& Tag) {
  UWorld* World = WorldContextObject->GetWorld();
  if (!IsValid(World))
    return nullptr;

  // Note: The actor iterator will be created with the
  // "EActorIteratorFlags::SkipPendingKill" flag,
  // meaning that we don't have to handle objects
  // that have been deleted. (This is the default,
  // but made explicit here)
  ACesiumGeoreference* Georeference = nullptr;
  EActorIteratorFlags flags = EActorIteratorFlags::OnlyActiveLevels |
                              EActorIteratorFlags::SkipPendingKill;
  for (TActorIterator<AActor> actorIterator(
           World,
           ACesiumGeoreference::StaticClass(),
           flags);
       actorIterator;
       ++actorIterator) {
    AActor* actor = *actorIterator;
    if (actor->GetLevel() == World->PersistentLevel &&
        actor->ActorHasTag(Tag)) {
      Georeference = Cast<ACesiumGeoreference>(actor);
      break;
    }
  }

  return Georeference;
}

ACesiumGeoreference*
FindGeoreferenceWithDefaultName(const UObject* WorldContextObject) {
  UWorld* World = WorldContextObject->GetWorld();
  if (!IsValid(World))
    return nullptr;

  ACesiumGeoreference* Candidate = FindObject<ACesiumGeoreference>(
      World->PersistentLevel,
      TEXT("CesiumGeoreferenceDefault"));

  // Test if PendingKill
  if (IsValid(Candidate)) {
    return Candidate;
  }

  return nullptr;
}

ACesiumGeoreference*
CreateDefaultGeoreference(const UObject* WorldContextObject, const FName& Tag) {
  UWorld* World = WorldContextObject->GetWorld();
  if (!IsValid(World))
    return nullptr;

  // Spawn georeference in the persistent level
  FActorSpawnParameters spawnParameters;
  spawnParameters.SpawnCollisionHandlingOverride =
      ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  spawnParameters.OverrideLevel = World->PersistentLevel;

  ACesiumGeoreference* Georeference =
      World->SpawnActor<ACesiumGeoreference>(spawnParameters);
  if (Georeference) {
    Georeference->Tags.Add(Tag);
  }

  return Georeference;
}

} // namespace

/*static*/ const double ACesiumGeoreference::kMinimumScale = 1.0e-6;

/*static*/ ACesiumGeoreference*
ACesiumGeoreference::GetDefaultGeoreference(const UObject* WorldContextObject) {
  if (!IsValid(WorldContextObject))
    return nullptr;

  ACesiumGeoreference* Georeference =
      FindGeoreferenceWithTag(WorldContextObject, DEFAULT_GEOREFERENCE_TAG);
  if (IsValid(Georeference))
    return Georeference;

  Georeference = FindGeoreferenceWithDefaultName(WorldContextObject);
  if (IsValid(Georeference))
    return Georeference;

  Georeference =
      CreateDefaultGeoreference(WorldContextObject, DEFAULT_GEOREFERENCE_TAG);

  return Georeference;
}

/*static*/ ACesiumGeoreference*
ACesiumGeoreference::GetDefaultGeoreferenceForActor(AActor* Actor) {
  if (!IsValid(Actor))
    return nullptr;

  ACesiumGeoreference* Georeference = FindGeoreferenceAncestor(Actor);
  if (IsValid(Georeference))
    return Georeference;

  return ACesiumGeoreference::GetDefaultGeoreference(Actor);
}

UCesiumEllipsoid* ACesiumGeoreference::GetEllipsoid() const {
  return this->Ellipsoid;
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
  return this->GetEllipsoid()
      ->LongitudeLatitudeHeightToEllipsoidCenteredEllipsoidFixed(
          this->GetOriginLongitudeLatitudeHeight());
}

void ACesiumGeoreference::SetOriginEarthCenteredEarthFixed(
    const FVector& TargetEarthCenteredEarthFixed) {
  this->SetOriginLongitudeLatitudeHeight(
      this->GetEllipsoid()
          ->EllipsoidCenteredEllipsoidFixedToLongitudeLatitudeHeight(
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
  return this->SubLevelCamera_DEPRECATED;
}

void ACesiumGeoreference::SetSubLevelCamera(APlayerCameraManager* NewValue) {
  this->SubLevelCamera_DEPRECATED = NewValue;
}

void ACesiumGeoreference::SetEllipsoid(UCesiumEllipsoid* NewEllipsoid) {
  UCesiumEllipsoid* OldEllipsoid = this->Ellipsoid;
  this->Ellipsoid = NewEllipsoid;
  this->OnEllipsoidChanged.Broadcast(OldEllipsoid, NewEllipsoid);
  this->UpdateGeoreference();
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
      this->GetEllipsoid()
          ->LongitudeLatitudeHeightToEllipsoidCenteredEllipsoidFixed(
              LongitudeLatitudeHeight));
}

FVector ACesiumGeoreference::TransformUnrealPositionToLongitudeLatitudeHeight(
    const FVector& UnrealPosition) const {
  return this->GetEllipsoid()
      ->EllipsoidCenteredEllipsoidFixedToLongitudeLatitudeHeight(
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
  return this
      ->ComputeEastSouthUpAtEarthCenteredEarthFixedPositionToUnrealTransformation(
          ecef);
}

FMatrix ACesiumGeoreference::
    ComputeEastSouthUpAtEarthCenteredEarthFixedPositionToUnrealTransformation(
        const FVector& EarthCenteredEarthFixedPosition) const {
  LocalHorizontalCoordinateSystem newLocal =
      this->GetEllipsoid()->CreateCoordinateSystem(
          EarthCenteredEarthFixedPosition,
          this->GetScale());
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
  if (!pWorld || !GEditor || pWorld->IsGameWorld()) {
    return;
  }

  this->Modify();

  FViewport* pViewport = GEditor->GetActiveViewport();
  FViewportClient* pViewportClient = pViewport->GetClient();
  FEditorViewportClient* pEditorViewportClient =
      static_cast<FEditorViewportClient*>(pViewportClient);

  // The viewport location / rotation is Unreal world coordinates. Transform
  // into the georeference's reference frame. This way we'll compute the correct
  // globe position even if the globe is transformed into the Unreal world.
  FVector ViewLocation = pEditorViewportClient->GetViewLocation();
  FQuat ViewRotation = pEditorViewportClient->GetViewRotation().Quaternion();

  ViewLocation =
      this->GetActorTransform().InverseTransformPosition(ViewLocation);
  ViewRotation =
      this->GetActorTransform().InverseTransformRotation(ViewRotation);

  FRotator NewViewRotation = this->TransformUnrealRotatorToEastSouthUp(
      ViewRotation.Rotator(),
      ViewLocation);

  // camera local space to ECEF
  FVector CameraEcefPosition =
      this->TransformUnrealPositionToEarthCenteredEarthFixed(ViewLocation);

  // Long/Lat/Height camera location, in degrees/meters (also our new target
  // georeference origin) When the location is too close to the center of the
  // earth, the result will be (0,0,0)
  this->SetOriginEarthCenteredEarthFixed(CameraEcefPosition);

  // TODO: check for degeneracy ?
  FVector cameraFront = NewViewRotation.RotateVector(FVector::XAxisVector);
  FVector cameraRight =
      FVector::CrossProduct(FVector::ZAxisVector, cameraFront).GetSafeNormal();
  FVector cameraUp =
      FVector::CrossProduct(cameraFront, cameraRight).GetSafeNormal();

  pEditorViewportClient->SetViewRotation(
      FMatrix(cameraFront, cameraRight, cameraUp, FVector::ZeroVector)
          .Rotator());
  pEditorViewportClient->SetViewLocation(
      this->GetActorTransform().TransformPosition(FVector::ZeroVector));
}

void ACesiumGeoreference::CreateSubLevelHere() {
  UWorld* World = GetWorld();
  if (!World || !GEditor || World->IsGameWorld()) {
    return;
  }

  // Deactivate any previous sub-levels, so that setting the georeference origin
  // doesn't change their origin, too.
  this->SubLevelSwitcher->SetTargetSubLevel(nullptr);

  // Update the georeference origin so that the new sub-level inherits it.
  this->PlaceGeoreferenceOriginHere();

  // Create a dummy Actor to add to the new sub-level, because
  // CreateLevelInstanceFrom forbids an empty array for some reason.
  TArray<AActor*> SubLevelActors;

  FActorSpawnParameters SpawnParameters{};
  SpawnParameters.NameMode =
      FActorSpawnParameters::ESpawnActorNameMode::Requested;
  SpawnParameters.Name = TEXT("Placeholder");
  SpawnParameters.ObjectFlags = RF_Transactional;

  AActor* PlaceholderActor = World->SpawnActor<AActor>(
      FVector::ZeroVector,
      FRotator::ZeroRotator,
      SpawnParameters);
  PlaceholderActor->SetActorLabel(TEXT("Placeholder"));

  SubLevelActors.Add(PlaceholderActor);

  // Create the new Level Instance Actor and corresponding streaming level.
  FNewLevelInstanceParams LevelInstanceParams{};
  LevelInstanceParams.PivotType = ELevelInstancePivotType::WorldOrigin;
  LevelInstanceParams.Type = ELevelInstanceCreationType::LevelInstance;
  LevelInstanceParams.SetExternalActors(false);

  ULevelInstanceSubsystem* LevelInstanceSubsystem =
      World->GetSubsystem<ULevelInstanceSubsystem>();
  AActor* LevelInstance =
      Cast<AActor>(LevelInstanceSubsystem->CreateLevelInstanceFrom(
          SubLevelActors,
          LevelInstanceParams));
  if (!IsValid(LevelInstance)) {
    // User canceled creation of the sub-level, so delete the placeholder we
    // created.
    PlaceholderActor->Destroy();
    return;
  }

  UCesiumSubLevelComponent* LevelComponent =
      Cast<UCesiumSubLevelComponent>(LevelInstance->AddComponentByClass(
          UCesiumSubLevelComponent::StaticClass(),
          false,
          FTransform::Identity,
          false));
  LevelComponent->SetFlags(RF_Transactional);
  LevelInstance->AddInstanceComponent(LevelComponent);

  // Move the new level instance under the CesiumGeoreference.
  LevelInstance->GetRootComponent()->SetMobility(
      this->GetRootComponent()->Mobility);
  LevelInstance->AttachToActor(
      this,
      FAttachmentTransformRules::KeepRelativeTransform);
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
       this->SubLevelSwitcher->GetRegisteredSubLevelsWeak()) {
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
}

void ACesiumGeoreference::Serialize(FArchive& Ar) {
  Super::Serialize(Ar);

  Ar.UsingCustomVersion(FCesiumCustomVersion::GUID);

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

  const int32 CesiumVersion =
      this->GetLinkerCustomVersion(FCesiumCustomVersion::GUID);

  if (CesiumVersion < FCesiumCustomVersion::OriginShiftComponent &&
      !this->SubLevelSwitcher->GetRegisteredSubLevelsWeak().IsEmpty()) {
    // In previous versions, the CesiumGeoreference managed origin shifting
    // based on a SubLevelCamera, which defaulted to the PlayerCameraManager of
    // the World's `GetFirstPlayerController()`.

    // Backward compatibility for this is tricky, but we can make a decent
    // attempt that will work in a lot of cases. And this is just an unfortunate
    // v2.0 breakage for any remaining cases.

    AActor* SubLevelActor = nullptr;

    if (this->SubLevelCamera_DEPRECATED) {
      // An explicit SubLevelCamera_DEPRECATED is specified. If it has a target
      // Actor, attach a CesiumOriginShiftComponent to that Actor.
      SubLevelActor = this->SubLevelCamera_DEPRECATED->ViewTarget.Target.Get();

      if (!SubLevelActor) {
        UE_LOG(
            LogCesium,
            Warning,
            TEXT("An explicit SubLevelCamera was specified on this "
                 "CesiumGeoreference, but its ViewTarget is not a valid "
                 "Actor, so a CesiumOriginShiftComponent could not be added "
                 "automatically. You must manually add a "
                 "CesiumOriginShiftComponent to the Actor whose position "
                 "should be used to control sub-level switching."));
      }
    } else {
      // No explicit SubLevelCamera_DEPRECATED, so try to find a Pawn set to
      // auto-possess player 0.
      for (TActorIterator<APawn> it(
               GetWorld(),
               APawn::StaticClass(),
               EActorIteratorFlags::SkipPendingKill);
           it;
           ++it) {
        if (it->AutoPossessPlayer == EAutoReceiveInput::Player0) {
          SubLevelActor = *it;
          break;
        }
      }

      if (!SubLevelActor) {
        UE_LOG(
            LogCesium,
            Warning,
            TEXT(
                "Could not find a Pawn in the level set to auto-possess player "
                "0, so a CesiumOriginShiftComponent could not be added "
                "automatically. You must manually add a "
                "CesiumOriginShiftComponent to the Actor whose position "
                "should be used to control sub-level switching."));
      }
    }

    if (SubLevelActor) {
      // If this is a Blueprint object, like DynamicPawn, its construction
      // scripts may not have been run yet at this point. Doing so might cause
      // an origin shift component to be added. So we force it to happen here so
      // that we don't end up adding a duplicate CesiumOriginShiftComponent.
      SubLevelActor->RerunConstructionScripts();
      if (SubLevelActor->FindComponentByClass<UCesiumOriginShiftComponent>() ==
          nullptr) {

        UCesiumOriginShiftComponent* OriginShift =
            Cast<UCesiumOriginShiftComponent>(
                SubLevelActor->AddComponentByClass(
                    UCesiumOriginShiftComponent::StaticClass(),
                    false,
                    FTransform::Identity,
                    false));
        OriginShift->SetFlags(RF_Transactional);
        SubLevelActor->AddInstanceComponent(OriginShift);

        UE_LOG(
            LogCesium,
            Warning,
            TEXT("Added CesiumOriginShiftComponent to %s in order to preserve "
                 "backward compatibility for sub-level switching."),
            *SubLevelActor->GetName());
      }
    }
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
  CESIUM_POST_EDIT_CHANGE(propertyName, ACesiumGeoreference, Ellipsoid);
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
      !this->GetLevel()->IsPersistentLevel())
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
    spawnParameters.NameMode =
        FActorSpawnParameters::ESpawnActorNameMode::Requested;
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

    // But if the georeference origin is close to this sub-level's origin,
    // make this the active sub-level.
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

  this->SubLevelSwitcher->SetTargetSubLevel(pActiveSubLevel);

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
  return this->GetEllipsoid()
      ->LongitudeLatitudeHeightToEllipsoidCenteredEllipsoidFixed(
          longitudeLatitudeHeight);
}

FVector ACesiumGeoreference::TransformEcefToLongitudeLatitudeHeight(
    const FVector& ecef) const {
  return this->GetEllipsoid()
      ->EllipsoidCenteredEllipsoidFixedToLongitudeLatitudeHeight(ecef);
}

FMatrix
ACesiumGeoreference::ComputeEastNorthUpToEcef(const FVector& ecef) const {
  return this->GetEllipsoid()->EastNorthUpToEllipsoidCenteredEllipsoidFixed(
      ecef);
}

ACesiumGeoreference::ACesiumGeoreference() : AActor() {
  struct FConstructorStatics {
    ConstructorHelpers::FObjectFinder<UCesiumEllipsoid> DefaultEllipsoid;

    FConstructorStatics()
        : DefaultEllipsoid(TEXT(
              "/Script/CesiumRuntime.CesiumEllipsoid'/CesiumForUnreal/WGS84.WGS84'")) {

    }
  };

  static FConstructorStatics ConstructorStatics;
  this->Ellipsoid = ConstructorStatics.DefaultEllipsoid.Object;

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
  if (IsValid(pSwitcher) && pSwitcher->GetTargetSubLevel() != nullptr) {
    if (pSwitcher->GetTargetSubLevel() == pSwitcher->GetCurrentSubLevel() ||
        pSwitcher->GetCurrentSubLevel() == nullptr) {
      ALevelInstance* pTarget = pSwitcher->GetTargetSubLevel();
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
      *this->GetEllipsoid()->GetNativeEllipsoid(),
      glm::dvec3(this->_coordinateSystem.getLocalToEcefTransformation()[3]),
      this->GetScale() / 100.0);
}

FName ACesiumGeoreference::DEFAULT_GEOREFERENCE_TAG =
    FName("DEFAULT_GEOREFERENCE");

void ACesiumGeoreference::_updateCoordinateSystem() {
  if (this->OriginPlacement == EOriginPlacement::CartographicOrigin) {
    FVector origin = this->GetOriginLongitudeLatitudeHeight();
    this->_coordinateSystem = this->GetEllipsoid()->CreateCoordinateSystem(
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
