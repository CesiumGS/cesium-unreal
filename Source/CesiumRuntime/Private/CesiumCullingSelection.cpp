// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumCullingSelection.h"
#include "StaticMeshResources.h"
#include <algorithm>
#include <numeric>

ACesiumCullingSelection::ACesiumCullingSelection() {
  PrimaryActorTick.bCanEverTick = true;
  
  this->Selection = 
    CreateDefaultSubobject<USplineComponent>(TEXT("CullingSelection"));
  
}

void ACesiumCullingSelection::OnConstruction(const FTransform& Transform) {
  if (!this->Georeference) {
    this->Georeference = ACesiumGeoreference::GetDefaultForActor(this);
  }
}

void ACesiumCullingSelection::BeginPlay() {
  if (!this->Georeference) {
    this->Georeference = ACesiumGeoreference::GetDefaultForActor(this);
  }
}

void ACesiumCullingSelection::UpdateCullingSelection() {
  int32 splinePointsCount = this->Selection->GetNumberOfSplinePoints();
  if (splinePointsCount < 3) {
    return;
  }
  std::vector<glm::dvec2> cartographicSelection(splinePointsCount);
  for (size_t i = 0; i < splinePointsCount; ++i) {
    const FVector& unrealPosition =
        this->Selection->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
    glm::dvec3 cartographic =
        this->Georeference->TransformUeToLongitudeLatitudeHeight(
            glm::dvec3(unrealPosition.X, unrealPosition.Y, unrealPosition.Z));
    cartographicSelection[i] = glm::dvec2(cartographic.x, cartographic.y);
  }

  // create ordered lists of indices into cartographicSelection
  std::vector<size_t> westToEast(splinePointsCount);
  std::iota(westToEast.begin(), westToEast.end(), 0);
  std::vector<size_t> southToNorth(westToEast);

  // weak lexicographic ordering from west to east, then south to north
  std::sort(
      westToEast.begin(),
      westToEast.end(),
      [cartographicSelection](size_t i0, size_t i1) -> bool {
        const glm::dvec2& first = cartographicSelection[i0];
        const glm::dvec2& second = cartographicSelection[i1];
        return first.x < second.x || first.x == second.x && first.y < second.y;
      });

  // weak ordering from south to north
  std::vector<glm::dvec2> southToNorth(cartographicSelection);
  std::sort(
      southToNorth.begin(),
      southToNorth.end(),
      [cartographicSelection](size_t i0, size_t i1) -> bool {
        const glm::dvec2& first = cartographicSelection[i0];
        const glm::dvec2& second = cartographicSelection[i1];
        return first.y <= second.y;
      });

  // the bounding globe rectangle
  double west = westToEast[0].x;
  double east = westToEast[splinePointsCount - 1].x;
  double south = southToNorth[0].x;
  double north = southToNorth[splinePointsCount - 1].x;

  // separate into trapezoids
  for (size_t i = 1; i < splinePointsCount - 1; ++i) {
    const glm::dvec2& current = westToEast[i];
  }
}