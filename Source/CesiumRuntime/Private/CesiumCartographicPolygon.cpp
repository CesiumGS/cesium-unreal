// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumCartographicPolygon.h"
#include "CesiumActors.h"
#include "CesiumUtility/Math.h"
#include "Components/SceneComponent.h"
#include "StaticMeshResources.h"
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

TSoftObjectPtr<ACesiumCartographicPolygon>
ACesiumCartographicPolygon::CreatePolygonFromGeoPoints(const TArray<FVector>& geoPoints) {
  if (geoPoints.IsEmpty()) {
    UE_LOG(LogTemp, Error, TEXT("Cannot create polygon from empty dataset."));
    return nullptr;
  }

  auto* world = GEngine->GetWorldContexts()[0].World();
  assert(world && "World not found");

  const auto* aPoly = world->SpawnActor<ACesiumCartographicPolygon>();
  const auto* georeference = aPoly->GlobeAnchor->ResolveGeoreference();

  TArray<FVector> unrealPoints;
  unrealPoints.Reserve(geoPoints.Num());

  FVector center {};
  for(const auto& p : geoPoints)
  {
    center += p;
    FVector unrealPosition = georeference->TransformLongitudeLatitudeHeightPositionToUnreal(p);
    unrealPoints.Add(unrealPosition);
  }
  center /= geoPoints.Num();

  aPoly->GlobeAnchor->MoveToLongitudeLatitudeHeight(center);
  aPoly->Polygon->SetSplinePoints(unrealPoints, ESplineCoordinateSpace::World);

  return TSoftObjectPtr<ACesiumCartographicPolygon>(aPoly);
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

void ACesiumCartographicPolygon::MakeLinear() const {
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
