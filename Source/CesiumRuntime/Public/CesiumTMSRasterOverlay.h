// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"

#include "CesiumRasterOverlay.h"
#include "Components/ActorComponent.h"
#include "CesiumTMSRasterOverlay.generated.h"

UCLASS(ClassGroup = (Cesium), meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumTMSRasterOverlay : public UCesiumRasterOverlay {
  GENERATED_BODY()

public:
  UPROPERTY(EditAnywhere, Category = "Cesium")
  FString SourceURL;

  /**
  * Whether to clamp zoom levels between MinimumLevel and MaximumLevel or
  automatically determine them from the source.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  bool bClampWithDefinedZoomLevels = false;

  /**
   * Minimum zoom level.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  int32 MinimumLevel = 0;

  /**
   * Maximum zoom level.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  int32 MaximumLevel = 10;

protected:
  virtual std::unique_ptr<Cesium3DTiles::RasterOverlay>
  CreateOverlay() override;
};
