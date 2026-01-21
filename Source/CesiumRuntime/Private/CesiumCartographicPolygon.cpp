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

#if WITH_EDITOR
#include "GameFramework/PlayerController.h"
#include "Editor/EditorEngine.h"
#endif

using namespace CesiumGeospatial;

bool getRayPlaneIntersection(const FVector& rayStart, const FVector& rayDir, const FPlane& targetPlane, /* out */ FVector& intersectionPoint)
{
  // Check if the ray and plane are parallel first to avoid division by zero
  if (FMath::Abs(FVector::DotProduct(rayDir, targetPlane.GetNormal())) < DBL_EPSILON)
  {
    UE_LOG(LogTemp, Warning, TEXT("Ray is parallel to the plane, no single intersection point."));
    return false;
  }

  double t = FMath::RayPlaneIntersectionParam(rayStart, rayDir, targetPlane);

  if (t > DBL_EPSILON)
  {
    intersectionPoint = rayStart + t * rayDir;
    return true;
  }

  // no intersection, or it is behind the viewer.
  return false;
}

bool getViewportCenterGroundIntersection(FVector& groundIntersection) {
  if (!GEditor)
    return false;

  UUnrealEditorSubsystem* editorSubsystem = GEditor->GetEditorSubsystem<UUnrealEditorSubsystem>();
  if (!editorSubsystem)
    return false;

  FVector viewPosition;
  FRotator viewRotation;

  editorSubsystem->GetLevelViewportCameraInfo(viewPosition, viewRotation);
  FVector viewDirection = viewRotation.Vector();

  FPlane groundPlane(FVector::UpVector);

  if (!getRayPlaneIntersection(viewPosition, viewDirection, groundPlane, groundIntersection)) {
    // view direction does not intersect ground plane
    return false;
  }

  return true;;
}

ACesiumCartographicPolygon::ACesiumCartographicPolygon() : AActor() {
  PrimaryActorTick.bCanEverTick = false;

  this->Polygon = CreateDefaultSubobject<USplineComponent>(TEXT("Selection"));
  this->SetRootComponent(this->Polygon);
  this->Polygon->SetClosedLoop(true);
  this->Polygon->SetMobility(EComponentMobility::Movable);
  FVector groundCenter {};

  TArray<FVector> points {
    FVector(-10000.0f, -10000.0f, 0.0f),
    FVector(10000.0f, -10000.0f, 0.0f),
    FVector(10000.0f, 10000.0f, 0.0f),
    FVector(-10000.0f, 10000.0f, 0.0f)};

  if (getViewportCenterGroundIntersection(groundCenter)) {
    for (FVector& point : points) {
      point += groundCenter;
    }
  }

  this->Polygon->SetSplinePoints(points,
      ESplineCoordinateSpace::Local);

  /*
  #include "UnrealEd/Public/LevelEditorViewport.h"
  #include "Editor/EditorEngine.h"
  #include "Editor.h"

  bool DeprojectEditorScreenToWorld(const FVector2D& ScreenPosition, FVector& WorldLocation, FVector& WorldDirection)
  {
  // Ensure the editor and an active viewport are available
  if (GEditor && GEditor->GetActiveViewport())
  {
  FLevelEditorViewportClient* ViewportClient = static_cast<FLevelEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient());

  if (ViewportClient)
  {
  // Use the ViewportClient's DeprojectScreenToWorld function
  // This function takes the screen position (pixel coordinates) and outputs the world location and direction
  return ViewportClient->DeprojectScreenToWorld(ScreenPosition, WorldLocation, WorldDirection);
  }
  }

  // Return false if the operation failed (e.g., no active viewport)
  return false;
  }
*/
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
