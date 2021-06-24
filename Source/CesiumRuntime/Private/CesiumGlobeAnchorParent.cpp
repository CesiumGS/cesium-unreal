// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGlobeAnchorParent.h"

#include "CesiumGeoreferenceComponent.h"
#include "Components/SceneComponent.h"

ADEPRECATED_CesiumGlobeAnchorParent::ADEPRECATED_CesiumGlobeAnchorParent() {
  PrimaryActorTick.bCanEverTick = true;

  // don't create the georeference root component if this is a CDO
  this->SetRootComponent(
      CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent")));
  this->RootComponent->SetMobility(EComponentMobility::Movable);
  this->GeoreferenceComponent =
      CreateDefaultSubobject<UCesiumGeoreferenceComponent>(
          TEXT("GeoreferenceComponent"));
}

void ADEPRECATED_CesiumGlobeAnchorParent::OnConstruction(
    const FTransform& Transform) {
  Super::OnConstruction(Transform);
  this->GeoreferenceComponent->SetAutoSnapToEastSouthUp(true);
}

bool ADEPRECATED_CesiumGlobeAnchorParent::ShouldTickIfViewportsOnly() const {
  return true;
}

void ADEPRECATED_CesiumGlobeAnchorParent::Tick(float DeltaTime) {

  // TODO GEOREF_REFACTORING This class is deprecated and
  // about to be removed.
  /*
  if (this->GeoreferenceComponent->CheckCoordinatesChanged()) {

    this->Longitude = this->GeoreferenceComponent->Longitude;
    this->Latitude = this->GeoreferenceComponent->Latitude;
    this->Height = this->GeoreferenceComponent->Height;

    this->ECEF_X = this->GeoreferenceComponent->ECEF_X;
    this->ECEF_Y = this->GeoreferenceComponent->ECEF_Y;
    this->ECEF_Z = this->GeoreferenceComponent->ECEF_Z;

    this->GeoreferenceComponent->MarkCoordinatesUnchanged();
  }
  */
}

#if WITH_EDITOR
void ADEPRECATED_CesiumGlobeAnchorParent::PostEditChangeProperty(
    FPropertyChangedEvent& event) {
  Super::PostEditChangeProperty(event);

  if (!event.Property) {
    return;
  }

  FName propertyName = event.Property->GetFName();

  // TODO GEOREF_REFACTORING This class is deprecated and
  // about to be removed.
  /*
  if (propertyName == GET_MEMBER_NAME_CHECKED(
                          ADEPRECATED_CesiumGlobeAnchorParent,
                          Longitude) ||
      propertyName == GET_MEMBER_NAME_CHECKED(
                          ADEPRECATED_CesiumGlobeAnchorParent,
                          Latitude) ||
      propertyName == GET_MEMBER_NAME_CHECKED(
                          ADEPRECATED_CesiumGlobeAnchorParent,
                          Height)) {
    this->GeoreferenceComponent->MoveToLongitudeLatitudeHeight(
        glm::dvec3(this->Longitude, this->Latitude, this->Height));
    return;
  } else if (
      propertyName == GET_MEMBER_NAME_CHECKED(
                          ADEPRECATED_CesiumGlobeAnchorParent,
                          ECEF_X) ||
      propertyName == GET_MEMBER_NAME_CHECKED(
                          ADEPRECATED_CesiumGlobeAnchorParent,
                          ECEF_Y) ||
      propertyName == GET_MEMBER_NAME_CHECKED(
                          ADEPRECATED_CesiumGlobeAnchorParent,
                          ECEF_Z)) {
    this->GeoreferenceComponent->MoveToECEF(
        glm::dvec3(this->ECEF_X, this->ECEF_Y, this->ECEF_Z));
    return;
  }
  */
}
#endif