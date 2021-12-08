// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "Cesium3DTilesetLoadFailureDetails.generated.h"

class ACesium3DTileset;

UENUM(BlueprintType)
enum class ECesium3DTilesetLoadType : uint8 {
  /**
   * A Cesium ion asset endpoint.
   */
  CesiumIon,

  /**
   * A tileset.json.
   */
  TilesetJson,

  /**
   * The content of a tile.
   */
  TileContent,

  /**
   * An implicit tiling subtree.
   *
   */
  TileSubtree,

  /**
   * An unknown load error.
   */
  Unknown
};

USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesium3DTilesetLoadFailureDetails {
  GENERATED_BODY()

  /**
   * The tileset that encountered the load failure.
   */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cesium")
  ACesium3DTileset* Tileset;

  /**
   * The type of request that failed to load.
   */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cesium")
  ECesium3DTilesetLoadType Type;

  /**
   * The HTTP status code of the response that led to the failure.
   *
   * If there was no response or the failure did not follow from a request, then
   * the value of this property will be 0.
   */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cesium")
  int32 HttpStatusCode;

  /**
   * A human-readable explanation of what failed.
   */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cesium")
  FString Message;
};
