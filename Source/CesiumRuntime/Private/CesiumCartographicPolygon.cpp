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
ACesiumCartographicPolygon::CreateClippingPolygon(TArray<FVector> points) {

/*
  // Bounding rect for Ion asset 3575638
  TArray<FVector> points {
    {56.120557, 25.292038, 0.000000},
    {56.120557, 25.295596, 0.000000},
    {56.124118, 25.295596, 0.000000},
    {56.124118, 25.292038, 0.000000}
  };
*/

  UE_LOG(LogTemp,Warning,TEXT("Found %d points"),points.Num());
  
  FVector center;
  // get bounding quad
  if (points.Num() > 0)
  {
    FVector mn = points[0];
    FVector mx = mn;
    
    for(int i=1; i < points.Num(); ++i)
    {
      UE_LOG(LogTemp,Warning,TEXT("%f, %f, %f"),points[i].X, points[i].Y, points[i].Z);

      mn.X = std::fmin(mn.X, points[i].X);
      mn.Y = std::fmin(mn.Y, points[i].Y);
      mn.Z = std::fmin(mn.Z, points[i].Z);
      
      mx.X = std::fmax(mx.X, points[i].X);
      mx.Y = std::fmax(mx.Y, points[i].Y);
      mx.Z = std::fmax(mx.Z, points[i].Z);
    }

    UE_LOG(LogTemp,Warning,TEXT("Min %f, %f, %f"),mn.X, mn.Y, mn.Z);
    UE_LOG(LogTemp,Warning,TEXT("Max %f, %f, %f"),mx.X, mx.Y, mx.Z);
    center = (mn + mx) / 2.0;
  }
  
  UE_LOG(LogTemp, Warning, TEXT("Center %f, %f, %f"), center.X, center.Y, center.Z);
  
  TArray<FVector> unrealPoints;
  
  auto* world = GEngine->GetWorldContexts()[0].World();
  assert(world && "Oh, no! Cannot find the world!");
  
  auto* result = world->SpawnActor<ACesiumCartographicPolygon>();
  result->GlobeAnchor->MoveToLongitudeLatitudeHeight(center);
  
  auto* georeference = result->GlobeAnchor->ResolveGeoreference();
  assert(georeference && "No valid Georeference found!");

  for(int i=0; i < points.Num(); ++i)
  {
    FVector unrealPosition = georeference->TransformLongitudeLatitudeHeightPositionToUnreal(points[i]);
    unrealPoints.Emplace(unrealPosition);
  }
  
  result->Polygon->SetSplinePoints(unrealPoints, ESplineCoordinateSpace::World);
  
  return result;
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
