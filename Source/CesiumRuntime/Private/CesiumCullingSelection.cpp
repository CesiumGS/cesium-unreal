// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumCullingSelection.h"
#include "CesiumUtility/Math.h"
#include "StaticMeshResources.h"
#include <algorithm>
#include <array>
#include <earcut.hpp>
#include <glm/glm.hpp>
#include <numeric>

#if WITH_EDITOR
#include "DrawDebugHelpers.h"
#endif

ACesiumCullingSelection::ACesiumCullingSelection() {
  PrimaryActorTick.bCanEverTick = true;

  this->Selection =
      CreateDefaultSubobject<USplineComponent>(TEXT("CullingSelection"));
  this->Selection->SetClosedLoop(true);
}

void ACesiumCullingSelection::OnConstruction(const FTransform& Transform) {
  if (!this->Georeference) {
    this->Georeference =
        ACesiumGeoreference::GetDefaultForActor(this);
  }
}

void ACesiumCullingSelection::BeginPlay() {
  if (!this->Georeference) {
    this->Georeference =
        ACesiumGeoreference::GetDefaultForActor(this);
  }
}

void ACesiumCullingSelection::UpdateCullingSelection() {
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

  const glm::dvec2& point0 = this->_cartographicSelection[0];

  // the bounding globe rectangle
  double west = point0.x;
  double east = point0.x;
  double south = point0.y;
  double north = point0.y;

  for (size_t i = 1; i < splinePointsCount; ++i) {
    const glm::dvec2& point1 = this->_cartographicSelection[i];

    if (point1.y > north) {
      north = point1.y;
    } else if (point1.y < south) {
      south = point1.y;
    }

    double dif_west = point1.x - west;
    // check if the difference crosses the antipole
    if (glm::abs(dif_west) > CesiumUtility::Math::ONE_PI) {
      // east wrapping past the antipole to the west
      if (dif_west > 0) {
        west = point1.x;
      }
    } else {
      if (dif_west < 0) {
        west = point1.x;
      }
    }

    double dif_east = point1.x - east;
    // check if the difference crosses the antipole
    if (glm::abs(dif_east) > CesiumUtility::Math::ONE_PI) {
      // west wrapping past the antipole to the east
      if (dif_east < 0) {
        east = point1.x;
      }
    } else {
      if (dif_east > 0) {
        east = point1.x;
      }
    }
  }

  this->_boundingRegion =
      CesiumGeospatial::GlobeRectangle(west, south, east, north);

  using Coord = double;
  using N = uint32_t;
  using Point = std::array<Coord, 2>;

  std::vector<std::vector<Point>> polygons;

  // normalize the longitude relative to the first point before triangulating
  std::vector<Point> polygon(splinePointsCount);
  polygon[0] = {0.0, point0.y};
  for (size_t i = 1; i < splinePointsCount; ++i) {
    const glm::dvec2& cartographic = this->_cartographicSelection[i];
    Point& point = polygon[i];
    point[0] = cartographic.x - point0.x;
    point[1] = cartographic.y;

    // check if the difference crosses the antipole
    if (glm::abs(point[0]) > CesiumUtility::Math::ONE_PI) {
      if (point[0] > 0) {
        point[0] -= CesiumUtility::Math::TWO_PI;
      } else {
        point[0] += CesiumUtility::Math::TWO_PI;
      }
    }
  }
  polygons.push_back(polygon);

  this->_indices = mapbox::earcut<N>(polygons);
}

bool ACesiumCullingSelection::ShouldTickIfViewportsOnly() const { return true; }

void ACesiumCullingSelection::Tick(float DeltaTime) {
#if WITH_EDITOR
  if (this->_boundingRegion) {
    // draw globe rectangle bounds
    this->_drawDebugLine(
        glm::dvec2(
            this->_boundingRegion->getWest(),
            this->_boundingRegion->getSouth()),
        glm::dvec2(
            this->_boundingRegion->getWest(),
            this->_boundingRegion->getNorth()));
    this->_drawDebugLine(
        glm::dvec2(
            this->_boundingRegion->getWest(),
            this->_boundingRegion->getNorth()),
        glm::dvec2(
            this->_boundingRegion->getEast(),
            this->_boundingRegion->getNorth()));
    this->_drawDebugLine(
        glm::dvec2(
            this->_boundingRegion->getEast(),
            this->_boundingRegion->getNorth()),
        glm::dvec2(
            this->_boundingRegion->getEast(),
            this->_boundingRegion->getSouth()));
    this->_drawDebugLine(
        glm::dvec2(
            this->_boundingRegion->getEast(),
            this->_boundingRegion->getSouth()),
        glm::dvec2(
            this->_boundingRegion->getWest(),
            this->_boundingRegion->getSouth()));
  }

  for (size_t i = 0; i < this->_indices.size(); ++i) {
    const glm::dvec2& a = this->_cartographicSelection[this->_indices[i]];
    const glm::dvec2& b = this->_cartographicSelection
                              [this->_indices[(i % 3 == 2) ? i - 2 : i + 1]];
    this->_drawDebugLine(a, b, 900.0, FColor::Blue);
  }
#endif
}

#if WITH_EDITOR
void ACesiumCullingSelection::_drawDebugLine(
    const glm::dvec2& point0,
    const glm::dvec2& point1,
    double height,
    FColor color) const {
  glm::dvec3 a = this->Georeference->TransformLongitudeLatitudeHeightToUe(
      glm::dvec3(glm::degrees(point0.x), glm::degrees(point0.y), height));
  glm::dvec3 b = this->Georeference->TransformLongitudeLatitudeHeightToUe(
      glm::dvec3(glm::degrees(point1.x), glm::degrees(point1.y), height));

  DrawDebugLine(
      this->GetWorld(),
      FVector(a.x, a.y, a.z),
      FVector(b.x, b.y, b.z),
      color,
      false,
      -1.0f,
      0.0f,
      500.0f);
}
#endif