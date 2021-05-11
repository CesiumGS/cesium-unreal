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

    /**
     * The shape to be culled out from the owning Cesium3DTileset.
     */
    UPROPERTY(EditAnywhere, Category="Cesium")
    UStaticMesh* CullingShape;

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
    
protected:

  virtual void BeginPlay() override;

private:
};
