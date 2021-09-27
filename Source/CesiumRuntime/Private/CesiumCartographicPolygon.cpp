// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumCartographicPolygon.h"
#include "CesiumUtility/Math.h"
#include "Components/SceneComponent.h"
#include "StaticMeshResources.h"
#include <glm/glm.hpp>

using namespace CesiumGeospatial;

ACesiumCartographicPolygon::ACesiumCartographicPolygon() {
  PrimaryActorTick.bCanEverTick = false;

  this->Polygon = CreateDefaultSubobject<USplineComponent>(TEXT("Selection"));
  this->SetRootComponent(this->Polygon);
  this->Polygon->SetClosedLoop(true);
  this->Polygon->SetMobility(EComponentMobility::Movable);

  this->Polygon->SetSplinePoints(
      TArray<FVector>{
          FVector(-10000.0f, -10000.0f, 0.0f),
          FVector(10000.0f, -10000.0f, 0.0f),
          FVector(10000.0f, 10000.0f, 0.0f),
          FVector(-10000.0f, 10000.0f, 0.0f)},
      ESplineCoordinateSpace::Local);

  this->MakeLinear();

  this->GlobeAnchor =
      CreateDefaultSubobject<UCesiumGlobeAnchorComponent>(TEXT("GlobeAnchor"));
}

void ACesiumCartographicPolygon::OnConstruction(const FTransform& Transform) {
  if (!this->Georeference) {
    this->Georeference = ACesiumGeoreference::GetDefaultGeoreference(this);
  }

  this->MakeLinear();
}

void ACesiumCartographicPolygon::BeginPlay() {
  if (!this->Georeference) {
    this->Georeference = ACesiumGeoreference::GetDefaultGeoreference(this);
  }

  this->MakeLinear();
}

CesiumGeospatial::CartographicPolygon
ACesiumCartographicPolygon::CreateCartographicPolygon() const {
  int32 splinePointsCount = this->Polygon->GetNumberOfSplinePoints();

  if (splinePointsCount < 3) {
    return CartographicPolygon({});
  }

  std::vector<glm::dvec2> polygon(splinePointsCount);

  for (size_t i = 0; i < splinePointsCount; ++i) {
    const FVector& unrealPosition = this->Polygon->GetLocationAtSplinePoint(
        i,
        ESplineCoordinateSpace::World);
    glm::dvec3 cartographic =
        this->Georeference->TransformUnrealToLongitudeLatitudeHeight(
            glm::dvec3(unrealPosition.X, unrealPosition.Y, unrealPosition.Z));
    polygon[i] =
        glm::dvec2(glm::radians(cartographic.x), glm::radians(cartographic.y));
  }

  return CartographicPolygon(polygon);
}

void ACesiumCartographicPolygon::MakeLinear() {
  // set spline point types to linear for all points.
  for (size_t i = 0; i < this->Polygon->GetNumberOfSplinePoints(); ++i) {
    this->Polygon->SetSplinePointType(i, ESplinePointType::Linear);
  }
}
