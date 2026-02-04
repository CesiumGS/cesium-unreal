// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumCartographicPolygon.h"
#include "CesiumActors.h"
#include "CesiumGeoreference.h"
#include "Components/SceneComponent.h"
#include "StaticMeshResources.h"
#if WITH_EDITOR
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "EditorViewportClient.h"
#include "Subsystems/UnrealEditorSubsystem.h"
#endif
#include "CesiumRuntime.h"
#include <glm/glm.hpp>

using namespace CesiumGeospatial;

#if WITH_EDITOR
bool ACesiumCartographicPolygon::ResetSplineAndCenterInEditorViewport() {
  if (!GEditor) {
    UE_LOG(LogCesium, Error, TEXT("Could not retrieve GEditor instance."));
    return false;
  }

  UUnrealEditorSubsystem* pEditorSubsystem =
      GEditor->GetEditorSubsystem<UUnrealEditorSubsystem>();
  if (!pEditorSubsystem) {
    UE_LOG(LogCesium, Error, TEXT("Could not retrieve Editor subsystem."));
    return false;
  }

  FVector viewPosition{};
  FRotator viewRotation{};

  if (!pEditorSubsystem->GetLevelViewportCameraInfo(
          viewPosition,
          viewRotation)) {
    UE_LOG(LogCesium, Error, TEXT("Could not retrieve viewport camera info."));
    return false;
  }

  // raycast the active viewport view ray to find its intersection with the
  // terrain.
  const FVector viewDirection = viewRotation.Vector();
  constexpr float traceDistance = 10000000.0f;
  FVector rayEnd = viewPosition + (viewDirection * traceDistance);

  FCollisionQueryParams traceParams{};
  traceParams.bTraceComplex = true;
  traceParams.bReturnPhysicalMaterial = true;

  FHitResult hitResult{};

  bool hit = GEditor->GetEditorWorldContext().World()->LineTraceSingleByChannel(
      hitResult,
      viewPosition,
      rayEnd,
      ECC_WorldStatic,
      traceParams);

  FVector spawnPosition;
  float extent;
  if (hit) {
    extent = hitResult.Distance / 2.0f;
    spawnPosition = hitResult.Location;
  } else {
    // no intersection detected, so create the polygon just in front of the
    // camera.
    extent = 1000.0f;
    spawnPosition = viewPosition + viewDirection * extent * 2.0f;
  }

  const TArray<FVector> points = {
      {-extent, -extent, 0},
      {extent, -extent, 0},
      {extent, extent, 0},
      {-extent, extent, 0},
  };

  this->Polygon->SetSplinePoints(points, ESplineCoordinateSpace::Local);

  this->MakeLinear();
  this->SetActorLocation(spawnPosition);

  return true;
}
#endif

ACesiumCartographicPolygon::ACesiumCartographicPolygon() : AActor() {
  PrimaryActorTick.bCanEverTick = false;

  this->Polygon = CreateDefaultSubobject<USplineComponent>(TEXT("Selection"));
  this->SetRootComponent(this->Polygon);
  this->Polygon->SetClosedLoop(true);
  this->Polygon->SetMobility(EComponentMobility::Movable);

  const TArray<FVector> points = {
      {-10000.0f, -10000.0f, 0},
      {10000.0f, -10000.0f, 0},
      {10000.0f, 10000.0f, 0},
      {-10000.0f, 10000.0f, 0},
  };

  this->Polygon->SetSplinePoints(points, ESplineCoordinateSpace::Local);
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
