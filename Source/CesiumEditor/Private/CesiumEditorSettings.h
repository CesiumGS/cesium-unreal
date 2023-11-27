// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumIonServer.h"
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CesiumEditorSettings.generated.h"

/**
 * Stores Editor settings for the Cesium plugin.
 */
UCLASS(Config = EditorPerProjectUserSettings, meta = (DisplayName = "Cesium"))
class UCesiumEditorSettings : public UDeveloperSettings {
  GENERATED_UCLASS_BODY()

public:
  UPROPERTY(
      Config,
      meta =
          (DeprecatedProperty,
           DeprecationMessage = "Set UserAccessTokenMap instead."))
  FString UserAccessToken_DEPRECATED;

  /**
   * The Cesium ion server that is currently selected in the user interface.
   */
  UPROPERTY(
      Config,
      EditAnywhere,
      Category = "Cesium ion",
      meta = (DisplayName = "Current Cesium ion Server"))
  TSoftObjectPtr<UCesiumIonServer> CurrentCesiumIonServer;

  UPROPERTY(
      Config,
      EditAnywhere,
      Category = "Cesium ion",
      meta = (DisplayName = "Token Map"))
  TMap<TSoftObjectPtr<UCesiumIonServer>, FString> UserAccessTokenMap;

  void Save();
};
