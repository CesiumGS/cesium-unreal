#include "CesiumSubLevelComponent.h"
#include "CesiumActors.h"
#include "CesiumGeoreference.h"
#include "CesiumGeospatial/LocalHorizontalCoordinateSystem.h"
#include "CesiumRuntime.h"
#include "CesiumSubLevelSwitcherComponent.h"
#include "CesiumUtility/Math.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "VecMath.h"

#if WITH_EDITOR
#include "EditorViewportClient.h"
#include "ScopedTransaction.h"
#endif

using namespace CesiumGeospatial;

bool UCesiumSubLevelComponent::GetEnabled() const { return this->Enabled; }

void UCesiumSubLevelComponent::SetEnabled(bool value) { this->Enabled = value; }

double UCesiumSubLevelComponent::GetOriginLongitude() const {
  return this->OriginLongitude;
}

void UCesiumSubLevelComponent::SetOriginLongitude(double value) {
  this->OriginLongitude = value;
  this->UpdateGeoreferenceIfSubLevelIsActive();
}

double UCesiumSubLevelComponent::GetOriginLatitude() const {
  return this->OriginLatitude;
}

void UCesiumSubLevelComponent::SetOriginLatitude(double value) {
  this->OriginLatitude = value;
  this->UpdateGeoreferenceIfSubLevelIsActive();
}

double UCesiumSubLevelComponent::GetOriginHeight() const {
  return this->OriginHeight;
}

void UCesiumSubLevelComponent::SetOriginHeight(double value) {
  this->OriginHeight = value;
  this->UpdateGeoreferenceIfSubLevelIsActive();
}

double UCesiumSubLevelComponent::GetLoadRadius() const {
  return this->LoadRadius;
}

void UCesiumSubLevelComponent::SetLoadRadius(double value) {
  this->LoadRadius = value;
}

TSoftObjectPtr<ACesiumGeoreference>
UCesiumSubLevelComponent::GetGeoreference() const {
  return this->Georeference;
}

void UCesiumSubLevelComponent::SetGeoreference(
    TSoftObjectPtr<ACesiumGeoreference> NewGeoreference) {
  this->Georeference = NewGeoreference;
  this->InvalidateResolvedGeoreference();

  ALevelInstance* pOwner = this->_getLevelInstance();
  if (pOwner) {
    this->ResolveGeoreference();

    UCesiumSubLevelSwitcherComponent* pSwitcher = this->_getSwitcher();
    pSwitcher->RegisterSubLevel(pOwner);
  }
}

ACesiumGeoreference* UCesiumSubLevelComponent::GetResolvedGeoreference() const {
  return this->ResolvedGeoreference;
}

ACesiumGeoreference* UCesiumSubLevelComponent::ResolveGeoreference() {
  if (IsValid(this->ResolvedGeoreference)) {
    return this->ResolvedGeoreference;
  }

  if (IsValid(this->Georeference.Get())) {
    this->ResolvedGeoreference = this->Georeference.Get();
  } else {
    this->ResolvedGeoreference =
        ACesiumGeoreference::GetDefaultGeoreference(this);
  }

  return this->ResolvedGeoreference;
}

void UCesiumSubLevelComponent::InvalidateResolvedGeoreference() {
  if (IsValid(this->ResolvedGeoreference)) {
    UCesiumSubLevelSwitcherComponent* pSwitcher = this->_getSwitcher();
    if (pSwitcher) {
      ALevelInstance* pOwner = this->_getLevelInstance();
      if (pOwner) {
        pSwitcher->UnregisterSubLevel(Cast<ALevelInstance>(pOwner));
      }
    }
  }
  this->ResolvedGeoreference = nullptr;
}

void UCesiumSubLevelComponent::SetOriginLongitudeLatitudeHeight(
    const FVector& longitudeLatitudeHeight) {
  if (this->OriginLongitude != longitudeLatitudeHeight.X ||
      this->OriginLatitude != longitudeLatitudeHeight.Y ||
      this->OriginHeight != longitudeLatitudeHeight.Z) {
    this->OriginLongitude = longitudeLatitudeHeight.X;
    this->OriginLatitude = longitudeLatitudeHeight.Y;
    this->OriginHeight = longitudeLatitudeHeight.Z;
    this->UpdateGeoreferenceIfSubLevelIsActive();
  }
}

#if WITH_EDITOR

void UCesiumSubLevelComponent::PlaceGeoreferenceOriginAtSubLevelOrigin() {
  ACesiumGeoreference* pGeoreference = this->ResolveGeoreference();
  if (!IsValid(pGeoreference)) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Cannot place the origin because the sub-level does not have a CesiumGeoreference."));
    return;
  }

  AActor* pOwner = this->GetOwner();

  if (!IsValid(pOwner)) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT("CesiumSubLevelComponent does not have an owning Actor."));
    return;
  }

  // Another sub-level might be active right now, so we construct the correct
  // GeoTransforms instead of using the CesiumGeoreference's.
  const Ellipsoid& ellipsoid = CesiumGeospatial::Ellipsoid::WGS84;
  glm::dvec3 originEcef =
      ellipsoid.cartographicToCartesian(Cartographic::fromDegrees(
          this->OriginLongitude,
          this->OriginLatitude,
          this->OriginHeight));
  GeoTransforms currentTransforms(
      ellipsoid,
      originEcef,
      pGeoreference->GetScale() / 100.0);

  // Construct new geotransforms at the new origin
  glm::dvec3 levelCenterEcef = currentTransforms.TransformUnrealToEcef(
      glm::dvec3(CesiumActors::getWorldOrigin4D(pOwner)),
      VecMath::createVector3D(pOwner->GetActorLocation()));

  std::optional<Cartographic> maybeCartographic =
      ellipsoid.cartesianToCartographic(levelCenterEcef);
  if (!maybeCartographic) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Cannot place the origin because the level instance's position on the globe cannot be converted to longitude/latitude/height. It may be too close to the center of the Earth."));
    return;
  }

  GeoTransforms newTransforms(
      ellipsoid,
      levelCenterEcef,
      pGeoreference->GetScale() / 100.0);

  // Transform the level instance from the old origin to the new one.
  glm::dmat4 oldToEcef =
      currentTransforms.GetAbsoluteUnrealWorldToEllipsoidCenteredTransform();
  glm::dmat4 ecefToNew =
      newTransforms.GetEllipsoidCenteredToAbsoluteUnrealWorldTransform();
  glm::dmat4 oldToNew = ecefToNew * oldToEcef;
  glm::dmat4 oldTransform =
      VecMath::createMatrix4D(pOwner->GetActorTransform().ToMatrixWithScale());
  glm::dmat4 newTransform = oldToNew * oldTransform;

  FScopedTransaction transaction(
      FText::FromString("Place Georeference Origin At SubLevel Origin"));

  pOwner->Modify();
  pOwner->SetActorTransform(FTransform(VecMath::createMatrix(newTransform)));

  // Set the new sub-level georeference origin.
  this->Modify();
  this->SetOriginLongitudeLatitudeHeight(FVector(
      CesiumUtility::Math::radiansToDegrees(maybeCartographic->longitude),
      CesiumUtility::Math::radiansToDegrees(maybeCartographic->latitude),
      maybeCartographic->height));

  // Also update the viewport so the level doesn't appear to shift.
  FViewport* pViewport = GEditor->GetActiveViewport();
  FViewportClient* pViewportClient = pViewport->GetClient();
  FEditorViewportClient* pEditorViewportClient =
      static_cast<FEditorViewportClient*>(pViewportClient);

  glm::dvec3 viewLocation =
      VecMath::createVector3D(pEditorViewportClient->GetViewLocation());
  viewLocation = glm::dvec3(oldToNew * glm::dvec4(viewLocation, 1.0));
  pEditorViewportClient->SetViewLocation(VecMath::createVector(viewLocation));

  glm::dmat4 viewportRotation = VecMath::createMatrix4D(
      pEditorViewportClient->GetViewRotation().Quaternion().ToMatrix());
  viewportRotation = oldToNew * viewportRotation;

  // At this point, viewportRotation will keep the viewport orientation in ECEF
  // exactly as it was before. But that means if it was tilted before, it will
  // still be tilted. We instead want an orientation that maintains the exact
  // same forward direction but has an "up" direction aligned with +Z.
  glm::dvec3 cameraFront = glm::normalize(glm::dvec3(viewportRotation[0]));
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
}

#endif // #if WITH_EDITOR

void UCesiumSubLevelComponent::UpdateGeoreferenceIfSubLevelIsActive() {
  ALevelInstance* pOwner = this->_getLevelInstance();
  if (!pOwner) {
    return;
  }

  if (!IsValid(this->ResolvedGeoreference)) {
    // This sub-level is not associated with a georeference yet.
    return;
  }

  UCesiumSubLevelSwitcherComponent* pSwitcher = this->_getSwitcher();
  if (!pSwitcher)
    return;

  ALevelInstance* pCurrent = pSwitcher->GetCurrentSubLevel();
  ALevelInstance* pTarget = pSwitcher->GetTargetSubLevel();

  // This sub-level's origin is active if it is the current level or if it's the
  // target level and there is no current level.
  if (pCurrent == pOwner || (pCurrent == nullptr && pTarget == pOwner)) {
    // Apply the sub-level's origin to the georeference, if it's different.
    if (this->OriginLongitude !=
            this->ResolvedGeoreference->GetOriginLongitude() ||
        this->OriginLatitude !=
            this->ResolvedGeoreference->GetOriginLatitude() ||
        this->OriginHeight != this->ResolvedGeoreference->GetOriginHeight()) {
      this->ResolvedGeoreference->SetOriginLongitudeLatitudeHeight(FVector(
          this->OriginLongitude,
          this->OriginLatitude,
          this->OriginHeight));
    }
  }
}

void UCesiumSubLevelComponent::BeginDestroy() {
  this->InvalidateResolvedGeoreference();
  Super::BeginDestroy();
}

void UCesiumSubLevelComponent::OnComponentCreated() {
  Super::OnComponentCreated();

  this->ResolveGeoreference();

  UCesiumSubLevelSwitcherComponent* pSwitcher = this->_getSwitcher();
  if (pSwitcher && this->ResolvedGeoreference) {
    this->OriginLongitude = this->ResolvedGeoreference->GetOriginLongitude();
    this->OriginLatitude = this->ResolvedGeoreference->GetOriginLatitude();
    this->OriginHeight = this->ResolvedGeoreference->GetOriginHeight();

    // In Editor worlds, make the newly-created sub-level the active one. Unless
    // it's already hidden.
#if WITH_EDITOR
    if (GEditor && IsValid(this->GetWorld()) &&
        !this->GetWorld()->IsGameWorld()) {
      ALevelInstance* pOwner = Cast<ALevelInstance>(this->GetOwner());
      if (IsValid(pOwner) && !pOwner->IsTemporarilyHiddenInEditor(true)) {
        pSwitcher->SetTargetSubLevel(pOwner);
      }
    }
#endif
  }
}

#if WITH_EDITOR

void UCesiumSubLevelComponent::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) {
  Super::PostEditChangeProperty(PropertyChangedEvent);

  if (!PropertyChangedEvent.Property) {
    return;
  }

  FName propertyName = PropertyChangedEvent.Property->GetFName();

  if (propertyName ==
          GET_MEMBER_NAME_CHECKED(UCesiumSubLevelComponent, OriginLongitude) ||
      propertyName ==
          GET_MEMBER_NAME_CHECKED(UCesiumSubLevelComponent, OriginLatitude) ||
      propertyName ==
          GET_MEMBER_NAME_CHECKED(UCesiumSubLevelComponent, OriginHeight)) {
    this->UpdateGeoreferenceIfSubLevelIsActive();
  }
}

#endif

void UCesiumSubLevelComponent::BeginPlay() {
  Super::BeginPlay();

  this->ResolveGeoreference();

  UCesiumSubLevelSwitcherComponent* pSwitcher = this->_getSwitcher();
  if (!pSwitcher)
    return;

  ALevelInstance* pLevel = this->_getLevelInstance();
  if (!pLevel)
    return;

  pSwitcher->RegisterSubLevel(pLevel);
}

void UCesiumSubLevelComponent::OnRegister() {
  Super::OnRegister();

  // We set this to true here so that the CesiumEditorSubLevelMutex in the
  // CesiumEditor module is invoked for this component when the
  // ALevelInstance's visibility is toggled in the Editor.
  bRenderStateCreated = true;

  ALevelInstance* pOwner = this->_getLevelInstance();
  if (!pOwner) {
    return;
  }

#if WITH_EDITOR
  if (pOwner->GetIsSpatiallyLoaded() ||
      pOwner->DesiredRuntimeBehavior !=
          ELevelInstanceRuntimeBehavior::LevelStreaming) {
    pOwner->Modify();

    // Cesium sub-levels must not be loaded and unloaded by the World
    // Partition system.
    if (pOwner->GetIsSpatiallyLoaded()) {
      pOwner->SetIsSpatiallyLoaded(false);
    }

    // Cesium sub-levels must use LevelStreaming behavior). The default
    // (Partitioned), will dump the actors in the sub-level into the main
    // level, which will prevent us from being to turn the sub-level on and
    // off at runtime.
    pOwner->DesiredRuntimeBehavior =
        ELevelInstanceRuntimeBehavior::LevelStreaming;

    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "Cesium changed the \"Is Spatially Loaded\" or \"Desired Runtime Behavior\" "
            "settings on Level Instance %s in order to work as a Cesium sub-level. If "
            "you're using World Partition, you may need to reload the main level in order "
            "for these changes to take effect."),
        *pOwner->GetName());
  }
#endif

  this->ResolveGeoreference();

  UCesiumSubLevelSwitcherComponent* pSwitcher = this->_getSwitcher();
  if (pSwitcher)
    pSwitcher->RegisterSubLevel(pOwner);

  this->UpdateGeoreferenceIfSubLevelIsActive();
}

void UCesiumSubLevelComponent::OnUnregister() {
  Super::OnUnregister();

  ALevelInstance* pOwner = this->_getLevelInstance();
  if (!pOwner) {
    return;
  }

  UCesiumSubLevelSwitcherComponent* pSwitcher = this->_getSwitcher();
  if (pSwitcher)
    pSwitcher->UnregisterSubLevel(pOwner);
}

UCesiumSubLevelSwitcherComponent*
UCesiumSubLevelComponent::_getSwitcher() noexcept {
  // Ignore transient level instances, like those that are created when
  // dragging from Create Actors but before releasing the mouse button.
  if (!IsValid(this->ResolvedGeoreference) || this->HasAllFlags(RF_Transient))
    return nullptr;

  return this->ResolvedGeoreference
      ->FindComponentByClass<UCesiumSubLevelSwitcherComponent>();
}

ALevelInstance* UCesiumSubLevelComponent::_getLevelInstance() const noexcept {
  ALevelInstance* pOwner = Cast<ALevelInstance>(this->GetOwner());
  if (!pOwner) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "A CesiumSubLevelComponent can only be attached a LevelInstance Actor."));
  }
  return pOwner;
}
