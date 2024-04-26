// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CesiumRuntimeSettings.generated.h"

/**
 * Stores runtime settings for the Cesium plugin.
 */
UCLASS(Config = Engine, DefaultConfig, meta = (DisplayName = "Cesium"))
class CESIUMRUNTIME_API UCesiumRuntimeSettings : public UDeveloperSettings {
  GENERATED_UCLASS_BODY()

public:
  UPROPERTY(
      Config,
      meta =
          (DeprecatedProperty,
           DeprecationMessage =
               "Tokens are now configured on CesiumIonServer data assets."))
  FString DefaultIonAccessTokenId_DEPRECATED;

  UPROPERTY(
      Config,
      meta =
          (DeprecatedProperty,
           DeprecationMessage =
               "Tokens are now configured on CesiumIonServer data assets."))
  FString DefaultIonAccessToken_DEPRECATED;

  UPROPERTY(
      Config,
      EditAnywhere,
      Category = "Level of Detail",
      meta = (DisplayName = "Scale Level-of-Detail by Display DPI"))
  bool ScaleLevelOfDetailByDPI = true;

  /**
   * Uses Unreal's occlusion culling engine to drive Cesium 3D Tiles selection,
   * reducing the detail of tiles that are occluded by other objects in the
   * scene so that less data overall needs to be loaded and rendered.
   */
  UPROPERTY(Config, EditAnywhere, Category = "Experimental Feature Flags")
  bool EnableExperimentalOcclusionCullingFeature = false;

  /**
   * The number of requests to handle before each prune of old cached results
   * from the database.
   */
  UPROPERTY(
      Config,
      EditAnywhere,
      Category = "Cache",
      meta = (ConfigRestartRequired = true))
  int RequestsPerCachePrune = 10000;

  /**
   * The maximum number of items that should be kept in the Sqlite database
   * after pruning.
   */
  UPROPERTY(
      Config,
      EditAnywhere,
      Category = "Cache",
      meta = (ConfigRestartRequired = true))
  int MaxCacheItems = 4096;
};
