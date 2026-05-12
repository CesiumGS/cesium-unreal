// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "Cesium3DTilesetLoadFailureDetails.generated.h"

class ACesium3DTileset;

UENUM(BlueprintType)
enum class ECesium3DTilesetLoadType : uint8 {
  /**
   * An unknown load error.
   */
  Unknown,

  /**
   * A Cesium ion asset endpoint.
   */
  CesiumIon,

  /**
   * A tileset.json.
   */
  TilesetJson
};

USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesium3DTilesetLoadFailureDetails {
  GENERATED_BODY()

  /**
   * The tileset that encountered the load failure.
   */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cesium")
  TWeakObjectPtr<ACesium3DTileset> Tileset = nullptr;

  /**
   * The type of request that failed to load.
   */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cesium")
  ECesium3DTilesetLoadType Type = ECesium3DTilesetLoadType::Unknown;

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
