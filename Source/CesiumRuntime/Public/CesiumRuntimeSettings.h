// Copyright 2020-2021 CesiumGS, Inc. and Contributors

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
  /**
   * The ID of the default access token to use to access Cesium ion assets at
   * runtime. This property may be an empty string, in which case the ID is
   * found by searching the logged-in Cesium ion account for the
   * DefaultIonAccessToken.
   */
  UPROPERTY(
      Config,
      EditAnywhere,
      Category = "Cesium ion",
      meta = (DisplayName = "Default Cesium ion Access Token ID"))
  FString DefaultIonAccessTokenId;

  /**
   * The default token used to access Cesium ion assets at runtime. This token
   * is embedded in packaged games for use at runtime.
   */
  UPROPERTY(
      Config,
      EditAnywhere,
      Category = "Cesium ion",
      meta = (DisplayName = "Default Cesium ion Access Token"))
  FString DefaultIonAccessToken;

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
};
