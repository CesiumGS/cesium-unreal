// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGeoreference.h"
#include "CesiumGeoreferenceComponent.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "Components/SplineComponent.h"
#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include <vector>

#include "CesiumCartographicSelection.generated.h"

/**
 *
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

  UPROPERTY()
  ACesiumGeoreference* Georeference;

  UPROPERTY()
  USplineComponent* Selection;

  UPROPERTY()
  UCesiumGeoreferenceComponent* GeoreferenceComponent;

  virtual void OnConstruction(const FTransform& Transform) override;

  void UpdateSelection();

  const std::vector<glm::dvec2>& GetCartographicSelection() const {
    return this->_cartographicSelection;
  }

protected:
  virtual void BeginPlay() override;

private:
  std::vector<glm::dvec2> _cartographicSelection;
};
