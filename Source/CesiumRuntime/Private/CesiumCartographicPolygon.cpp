// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumCartographicPolygon.h"
#include "CesiumActors.h"
#include "CesiumUtility/Math.h"
#include "Components/SceneComponent.h"
#include "StaticMeshResources.h"
#include <CesiumUtility/Assert.h>
#include <glm/glm.hpp>

using namespace CesiumGeospatial;

ACesiumCartographicPolygon::ACesiumCartographicPolygon() : AActor() {
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
#if WITH_EDITOR
  this->SetIsSpatiallyLoaded(false);
#endif
  this->GlobeAnchor =
      CreateDefaultSubobject<UCesiumGlobeAnchorComponent>(TEXT("GlobeAnchor"));
}

void ACesiumCartographicPolygon::OnConstruction(const FTransform& Transform) {
  this->MakeLinear();
}

void ACesiumCartographicPolygon::BeginPlay() {
  Super::BeginPlay();
  this->MakeLinear();
}

void ACesiumCartographicPolygon::SetPolygonPoints(
    const ECesiumCoordinateReferenceSystem CoordinateReferenceSystem,
    const TArray<FVector>& Points) {
  if (Points.IsEmpty()) {
    UE_LOG(LogTemp, Error, TEXT("Points array cannot be empty"));
    return;
  }

  switch (CoordinateReferenceSystem) {
    case ECesiumCoordinateReferenceSystem::LongitudeLatitudeHeight:
    case ECesiumCoordinateReferenceSystem::EarthCenteredEarthFixed:
    break;
  default:
    UE_LOG(LogTemp, Error, TEXT("Unhandled CoordinateReferenceSystem value."));
    return;
  }

  TArray<FVector> unrealPoints;
  unrealPoints.Reserve(Points.Num());

  FVector center{};
  const ACesiumGeoreference* pGeoreference =
      this->GlobeAnchor->ResolveGeoreference();

  FVector unrealPosition{};
  for (const FVector& point : Points) {
    center += point;
    unrealPosition =
        (CoordinateReferenceSystem ==
         ECesiumCoordinateReferenceSystem::LongitudeLatitudeHeight)
            ? pGeoreference->TransformLongitudeLatitudeHeightPositionToUnreal(
                  point)
            : pGeoreference->TransformEarthCenteredEarthFixedPositionToUnreal(
                  point);
    unrealPoints.Add(unrealPosition);
  }
  center /= Points.Num();

  if (CoordinateReferenceSystem ==
      ECesiumCoordinateReferenceSystem::LongitudeLatitudeHeight) {
    this->GlobeAnchor->MoveToLongitudeLatitudeHeight(center);
  } else {
    this->GlobeAnchor->MoveToEarthCenteredEarthFixedPosition(center);
  }

  this->Polygon->SetSplinePoints(unrealPoints, ESplineCoordinateSpace::World);
}

CesiumGeospatial::CartographicPolygon
ACesiumCartographicPolygon::CreateCartographicPolygon(
    const FTransform& worldToTileset) const {
  int32 splinePointsCount = this->Polygon->GetNumberOfSplinePoints();

  if (splinePointsCount < 3) {
    return CartographicPolygon(std::vector<glm::dvec2>{});
  }

  std::vector<glm::dvec2> polygon(splinePointsCount);

  // The spline points should be located in the tileset _exactly where they
  // appear to be_. The way we do that is by getting their world position, and
  // then transforming that world position to a Cesium3DTileset local position.
  // That way if the tileset is transformed relative to the globe, the polygon
  // will still affect the tileset where the user thinks it should.

  for (size_t i = 0; i < splinePointsCount; ++i) {
    const FVector& unrealPosition = worldToTileset.TransformPosition(
        this->Polygon->GetLocationAtSplinePoint(
            i,
            ESplineCoordinateSpace::World));
    FVector cartographic =
        this->GlobeAnchor->ResolveGeoreference()
            ->TransformUnrealPositionToLongitudeLatitudeHeight(unrealPosition);
    polygon[i] =
        glm::dvec2(glm::radians(cartographic.X), glm::radians(cartographic.Y));
  }

  return CartographicPolygon(polygon);
}

void ACesiumCartographicPolygon::MakeLinear() {
  // set spline point types to linear for all points.
  for (size_t i = 0; i < this->Polygon->GetNumberOfSplinePoints(); ++i) {
    this->Polygon->SetSplinePointType(i, ESplinePointType::Linear);
  }
}

void ACesiumCartographicPolygon::PostLoad() {
  Super::PostLoad();

  if (CesiumActors::shouldValidateFlags(this))
    CesiumActors::validateActorFlags(this);
}
