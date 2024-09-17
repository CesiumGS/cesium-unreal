// Copyright 2020-2024 CesiumGS, Inc. and Contributors
#pragma once

#include "Kismet/BlueprintAsyncActionBase.h"
#include "CesiumSampleHeightMostDetailedAsyncAction.generated.h"

class ACesium3DTileset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCesiumSampleHeightMostDetailedComplete,
    const TArray<FVector>&,
    LongitudesLatitudesAndHeights,
    const TArray<FString>&,
    Warnings);

UCLASS()
class CESIUMRUNTIME_API UCesiumSampleHeightMostDetailedAsyncAction
    : public UBlueprintAsyncActionBase {
  GENERATED_BODY()

public:
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium",
      meta = (BlueprintInternalUseOnly = "true"))
  static UCesiumSampleHeightMostDetailedAsyncAction* SampleHeightMostDetailed(
      ACesium3DTileset* Tileset,
      const TArray<FVector2D>& longitudesAndLatitudes);

  UPROPERTY(BlueprintAssignable)
  FCesiumSampleHeightMostDetailedComplete OnFinished;

private:
  ACesium3DTileset* _pTileset;
  TArray<FVector2D> _longitudesAndLatitudes;
};
