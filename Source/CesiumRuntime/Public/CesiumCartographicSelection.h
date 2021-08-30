// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGeoreference.h"
#include "CesiumGeoreferenceComponent.h"
#include "CesiumGeospatial/CartographicPolygon.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "Components/SplineComponent.h"
#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include <vector>

#include "CesiumCartographicSelection.generated.h"

/**
 * A spline-based selection actor used to rasterize 2D texture masks on top of
 * ACesium3DTileset actors.
 */
UCLASS(ClassGroup = (Cesium), meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API ACesiumCartographicSelection : public AActor {

  GENERATED_BODY()

public:
  ACesiumCartographicSelection();

  /**
   * Whether this selection will be used to cull sections of tilesets.
   *
   * This options lets the tilesets know whether they should avoid loading
   * tiles that fall entirely within the selection.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  bool IsForCulling = true;

  /**
   * The target texture to rasterize the selection into.
   *
   * This texture name is to be used later in the material to refer to the
   * rasterized selection. All other selections included on the tileset with
   * the same texture name will end up rasterized in the same texture as well.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  FString TargetTexture = "Clipping";

  UPROPERTY()
  ACesiumGeoreference* Georeference;

  UPROPERTY()
  USplineComponent* Selection;

  UPROPERTY()
  UCesiumGeoreferenceComponent* GeoreferenceComponent;

  virtual void OnConstruction(const FTransform& Transform) override;

  /**
   * Turn the current georeferenced spline selection into a list of
   * cartographic coordinates which can be used i
   */
  void UpdateSelection();

  /**
   * Creates and returns a Cesium3DTilesSelection::CartographicSelection object
   * out of the current spline selection.
   */
  CesiumGeospatial::CartographicPolygon
  CreateCesiumCartographicSelection() const;

protected:
  virtual void BeginPlay() override;

private:
  std::vector<glm::dvec2> _cartographicSelection;
};
