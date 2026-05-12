// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "CesiumRasterOverlayLoadFailureDetails.generated.h"

class UCesiumRasterOverlay;

UENUM(BlueprintType)
enum class ECesiumRasterOverlayLoadType : uint8 {
  /**
   * An unknown load error.
   */
  Unknown,

  /**
   * A Cesium ion asset endpoint.
   */
  CesiumIon,

  /**
   * @brief An initial load needed to create the overlay's tile provider.
   */
  TileProvider
};

USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumRasterOverlayLoadFailureDetails {
  GENERATED_BODY()

  /**
   * The overlay that encountered the load failure.
   */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cesium")
  TWeakObjectPtr<UCesiumRasterOverlay> Overlay = nullptr;

  /**
   * The type of request that failed to load.
   */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cesium")
  ECesiumRasterOverlayLoadType Type = ECesiumRasterOverlayLoadType::Unknown;

  /**
   * The HTTP status code of the response that led to the failure.
   *
   * If there was no response or the failure did not follow from a request, then
   * the value of this property will be 0.
   */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cesium")
  int32 HttpStatusCode = 0;

  /**
   * A human-readable explanation of what failed.
   */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cesium")
  FString Message;
};
