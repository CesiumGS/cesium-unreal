#include "CesiumSubLevelComponent.h"
#include "CesiumGeoreference.h"
#include "CesiumSubLevelSwitcherComponent.h"
#include "LevelInstance/LevelInstanceActor.h"

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

ACesiumGeoreference* UCesiumSubLevelComponent::GetGeoreference() const {
  return this->Georeference;
}

void UCesiumSubLevelComponent::SetGeoreference(
    ACesiumGeoreference* NewGeoreference) {
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

  if (IsValid(this->Georeference)) {
    this->ResolvedGeoreference = this->Georeference;
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

  ALevelInstance* pCurrent = pSwitcher->GetCurrent();
  ALevelInstance* pTarget = pSwitcher->GetTarget();

  // This sub-level's origin is active if it is the current level or if it's the
  // target level and there is no current level.
  if (pCurrent == pOwner || (pCurrent == nullptr && pTarget == pOwner)) {
    // Apply the sub-level's origin to the georeference, if it's different.
    if (this->OriginLongitude != this->ResolvedGeoreference->OriginLongitude ||
        this->OriginLatitude != this->ResolvedGeoreference->OriginLatitude ||
        this->OriginHeight != this->ResolvedGeoreference->OriginHeight) {
      this->ResolvedGeoreference->SetGeoreferenceOriginLongitudeLatitudeHeight(
          glm::dvec3(
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
    this->OriginLongitude = this->ResolvedGeoreference->OriginLongitude;
    this->OriginLatitude = this->ResolvedGeoreference->OriginLatitude;
    this->OriginHeight = this->ResolvedGeoreference->OriginHeight;

    // Make the sub-level's position the georeference origin and reset the
    // origin to (0,0,0).
    // TODO: make this a button instead
    // AActor* pOwner = this->GetOwner();
    // const FTransform& transform = pOwner->GetActorTransform();
    // FVector location = transform.GetLocation();

    // FVector llh =
    //     this->ResolvedGeoreference->TransformUnrealToLongitudeLatitudeHeight(
    //         location);

    // this->OriginLongitude = llh.X;
    // this->OriginLatitude = llh.Y;
    // this->OriginHeight = llh.Z;

    // pOwner->SetActorLocation(FVector::ZeroVector);
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
  // CesiumEditor module is invoked for this component when the ALevelInstance's
  // visibility is toggled in the Editor.
  bRenderStateCreated = true;

  ALevelInstance* pOwner = this->_getLevelInstance();
  if (!pOwner) {
    return;
  }

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
  // Ignore transient level instances, like those that are created when dragging
  // from Create Actors but before releasing the mouse button.
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
