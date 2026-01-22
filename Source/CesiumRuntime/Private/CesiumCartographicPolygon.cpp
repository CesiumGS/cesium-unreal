// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumCartographicPolygon.h"
#include "CesiumActors.h"
#include "CesiumUtility/Math.h"
#include "Components/SceneComponent.h"
#include "StaticMeshResources.h"
#include "Subsystems/UnrealEditorSubsystem.h"
#include <CesiumUtility/Assert.h>
#include <ThirdParty/ShaderConductor/ShaderConductor/External/SPIRV-Headers/include/spirv/unified1/spirv.h>
#include <glm/glm.hpp>

using namespace CesiumGeospatial;

#include "Editor/UnrealEdEngine.h"
#include "Editor.h"
#include "LevelEditorViewport.h"

struct FViewportCornerInfo
{
  FVector WorldPosition;
  FVector Direction;
  FVector PlanePosition;
  bool hitsPlane;
};

bool GetEditorViewportCorners(TArray<FViewportCornerInfo>& Corners)
{
  #if WITH_EDITOR
  if (GEditor && GEditor->GetActiveViewport())
  {
    FViewport* ActiveViewport = GEditor->GetActiveViewport();
    FLevelEditorViewportClient* ViewportClient = nullptr;

    // Get the viewport client
    for (FLevelEditorViewportClient* Client : GEditor->GetLevelViewportClients())
    {
      if (Client && Client->Viewport == ActiveViewport)
      {
        ViewportClient = Client;
        break;
      }
    }

    if (ViewportClient)
    {
      FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
      ViewportClient->Viewport,
      ViewportClient->GetScene(),
      ViewportClient->EngineShowFlags)
      .SetRealtimeUpdate(true));

      FSceneView* View = ViewportClient->CalcSceneView(&ViewFamily);

      if (View)
      {
        FIntPoint ViewportSize = ActiveViewport->GetSizeXY();

        TArray<FVector2D> ScreenPositions = {
          FVector2D(0.1 * ViewportSize.X, 0.9 * ViewportSize.Y), // bottom left
          FVector2D(0.9 * ViewportSize.X, 0.9 * ViewportSize.Y), // bottom right
          FVector2D(0.9 * ViewportSize.X, 0.1 * ViewportSize.Y), // top right
          FVector2D(0.1 * ViewportSize.X, 0.1 * ViewportSize.Y), // top left right
          FVector2D(ViewportSize.X * 0.5f, ViewportSize.Y * 0.5f)  // Center
        };

        const FPlane groundPlane (FVector::UpVector, 0.0f);
        for (const FVector2D& ScreenPos : ScreenPositions)
        {
          FViewportCornerInfo Corner;

          // Deproject screen position to world space
          View->DeprojectFVector2D(ScreenPos, Corner.WorldPosition, Corner.Direction);

          Corner.hitsPlane = false;
          float denominator = FVector::DotProduct(Corner.Direction, groundPlane );
          if (FMath::Abs(denominator) > FLT_EPSILON) {
            float T = -(FVector::DotProduct(Corner.WorldPosition, groundPlane) + groundPlane.W) / denominator;
            Corner.hitsPlane = true;
            Corner.PlanePosition = Corner.WorldPosition + Corner.Direction * T;
          } else {
            // ray is parallel to ground or intersection is behind the camera.
            return false;
          }

          // Corner.Direction = Corner.Direction.GetSafeNormal();
          Corners.Add(Corner);
        }

        return true;
      }
    }
  }

  #endif

  return false;
}


ACesiumCartographicPolygon::ACesiumCartographicPolygon() : AActor() {
  PrimaryActorTick.bCanEverTick = false;

  this->Polygon = CreateDefaultSubobject<USplineComponent>(TEXT("Selection"));
  this->SetRootComponent(this->Polygon);
  this->Polygon->SetClosedLoop(true);
  this->Polygon->SetMobility(EComponentMobility::Movable);

  TArray<FVector> points {
    FVector(-10000.0f, -10000.0f, 0.0f),
    FVector(10000.0f, -10000.0f, 0.0f),
    FVector(10000.0f, 10000.0f, 0.0f),
    FVector(-10000.0f, 10000.0f, 0.0f)};

  TArray<FViewportCornerInfo> corners;

  if (GetEditorViewportCorners(corners))
  {
    float extent = FVector::Distance(corners[0].PlanePosition, corners[1].PlanePosition) * 0.9f;
    FVector center = corners[4].PlanePosition;
    points = {
    { -extent, -extent, 0 },
    { extent, -extent, 0 },
    { extent, extent, 0 },
    { -extent, extent, 0 },
    };
    //
    // for (FVector& point : points) {
    //   point += center;
    // }
    if (corners.Num() >= 4) {
      for (int i=0; i < 4; ++i) {
        points[i] += center;
        // FVector& p1 = points[i];
        // FViewportCornerInfo& corner = corners[i];
        // p1 = corner.PlanePosition;
      }
    }
  }

  this->Polygon->SetSplinePoints(points,
      ESplineCoordinateSpace::Local);

#if WITH_EDITOR
  if (GEditor)
    GEditor->RedrawAllViewports(true);
#endif

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
