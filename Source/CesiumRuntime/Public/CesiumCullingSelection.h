// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGeoreference.h"
#include "CesiumGeoreferenceComponent.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "Components/SplineComponent.h"
#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include <optional>
#include <vector>

#include "CesiumCullingSelection.generated.h"

/**
 *
 */
UCLASS(ClassGroup = (Cesium), meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API ACesiumCullingSelection : public AActor {

  GENERATED_BODY()

public:
  ACesiumCullingSelection();

  UPROPERTY()
  ACesiumGeoreference* Georeference;

  // TODO: maybe EditAnywhere is wrong here
  UPROPERTY()
  USplineComponent* Selection;

  UPROPERTY()
  UCesiumGeoreferenceComponent* GeoreferenceComponent;

  // TODO: triangulated vertices (and indices??) for closed-loop-splines

  // TODO: may be able to just do this onconstruction or posteditproperties
  // (when the spline has changed)
  UFUNCTION(CallInEditor, Category = "Cesium")
  void UpdateCullingSelection();

  virtual void OnConstruction(const FTransform& Transform) override;
  virtual bool ShouldTickIfViewportsOnly() const override;
  virtual void Tick(float DeltaTime) override;

  const std::optional<CesiumGeospatial::GlobeRectangle>&
  GetBoundingRegion() const {
    return this->_boundingRegion;
  }

  const std::vector<glm::dvec2>& GetCartographicSelection() const {
    return this->_cartographicSelection;
  }

  const std::vector<uint32_t>& GetTriangulatedIndices() const {
    return this->_indices;
  }

protected:
  virtual void BeginPlay() override;

private:
  std::optional<CesiumGeospatial::GlobeRectangle> _boundingRegion;

  std::vector<glm::dvec2> _cartographicSelection;
  std::vector<uint32_t> _indices;

#if WITH_EDITOR
  void _drawDebugLine(
      const glm::dvec2& point0,
      const glm::dvec2& point1,
      double height = 1000.0,
      FColor color = FColor::Red) const;
#endif
};
