// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumCartographicSelection.h"
#include "CesiumUtility/Math.h"
#include "Components/SceneComponent.h"
#include "StaticMeshResources.h"
#include <glm/glm.hpp>

ACesiumCartographicSelection::ACesiumCartographicSelection() {
  PrimaryActorTick.bCanEverTick = false;

  this->Selection = CreateDefaultSubobject<USplineComponent>(TEXT("Selection"));
  this->SetRootComponent(this->Selection);
  this->Selection->SetClosedLoop(true);
  this->Selection->SetMobility(EComponentMobility::Movable);

  this->GeoreferenceComponent =
      CreateDefaultSubobject<UCesiumGeoreferenceComponent>(
          TEXT("GeoreferenceComponent"));
}

void ACesiumCartographicSelection::OnConstruction(const FTransform& Transform) {
  if (!this->Georeference) {
    this->Georeference = ACesiumGeoreference::GetDefaultForActor(this);
  }

  this->UpdateSelection();
}

void ACesiumCartographicSelection::BeginPlay() {
  if (!this->Georeference) {
    this->Georeference = ACesiumGeoreference::GetDefaultForActor(this);
  }
}

void ACesiumCartographicSelection::UpdateSelection() {
  int32 splinePointsCount = this->Selection->GetNumberOfSplinePoints();

  if (splinePointsCount < 3) {
    return;
  }

  // set spline point types to linear for all points.
  for (size_t i = 0; i < splinePointsCount; ++i) {
    this->Selection->SetSplinePointType(i, ESplinePointType::Linear);
  }

  this->_cartographicSelection.clear();
  this->_cartographicSelection.resize(splinePointsCount);
  for (size_t i = 0; i < splinePointsCount; ++i) {
    const FVector& unrealPosition = this->Selection->GetLocationAtSplinePoint(
        i,
        ESplineCoordinateSpace::World);
    glm::dvec3 cartographic =
        this->Georeference->TransformUeToLongitudeLatitudeHeight(
            glm::dvec3(unrealPosition.X, unrealPosition.Y, unrealPosition.Z));
    this->_cartographicSelection[i] =
        glm::dvec2(glm::radians(cartographic.x), glm::radians(cartographic.y));
  }
}

CesiumGeospatial::CartographicPolygon
ACesiumCartographicSelection::CreateCesiumCartographicSelection() const {
  return CesiumGeospatial::CartographicPolygon(
      TCHAR_TO_UTF8(*this->TargetTexture),
      this->_cartographicSelection,
      this->IsForCulling);
}
