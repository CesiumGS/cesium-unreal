// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/StaticMesh.h"
#include "Components/SplineComponent.h"
#include "CesiumGeoreference.h"
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

    UPROPERTY(EditAnywhere, Category = "Cesium")
    ACesiumGeoreference* Georeference;

    // TODO: maybe EditAnywhere is wrong here
    UPROPERTY(EditAnywhere, Category = "Cesium")
    USplineComponent* Selection;

    // TODO: triangulated vertices (and indices??) for closed-loop-splines

    // TODO: may be able to just do this onconstruction or posteditproperties 
    // (when the spline has changed)
    UFUNCTION(CallInEditor, Category = "Cesium")
    void UpdateCullingSelection();

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual bool ShouldTickIfViewportsOnly() const override;
    virtual void Tick(float DeltaTime) override;
    
protected:

  virtual void BeginPlay() override;

private:
  // TEMP
  double west = 0.0;
  double east = 0.0;
  double south = 0.0;
  double north = 0.0;

  std::vector<glm::dvec2> cartographicSelection;
  std::vector<uint32_t> indices;

#if WITH_EDITOR
  void _drawDebugLine(const glm::dvec2& point0, const glm::dvec2& point1, double height = 1000.0, FColor color = FColor::Red) const;
 #endif
};
