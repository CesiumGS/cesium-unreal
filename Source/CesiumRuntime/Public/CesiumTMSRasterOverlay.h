// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"

#include "CesiumRasterOverlay.h"
#include "Components/ActorComponent.h"
#include "CesiumTMSRasterOverlay.generated.h"


UCLASS(ClassGroup=(Cesium), meta=(BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumTMSRasterOverlay : public UCesiumRasterOverlay {
  GENERATED_BODY()

public:
  UPROPERTY(EditAnywhere, Category = "Cesium")
    FString SourceURL;
  
  // Minimum zoom level of the raster tiles. If MaximumLevel <= MinimumLevel, the provider will attempt to determine these values.
  UPROPERTY(EditAnywhere, Category = "Cesium")
  	int32 MinimumLevel = 0;

  // Maximum zoom level of the raster tiles.
  UPROPERTY(EditAnywhere, Category = "Cesium")
  	int32 MaximumLevel = 10;

protected:
  virtual std::unique_ptr<Cesium3DTiles::RasterOverlay>
  CreateOverlay() override;
  
};
