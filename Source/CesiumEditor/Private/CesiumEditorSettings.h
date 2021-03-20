// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CesiumEditorSettings.generated.h"

/**
 * Stores settings for the Cesium Editor module.
 */
UCLASS(Config = Cesium)
class UCesiumEditorSettings : public UDeveloperSettings {
  GENERATED_UCLASS_BODY()

public:
  /**
   * The token used to access Cesium ion. If this is blank or invalid, the
   * Cesium panel will prompt you to log in to Cesium ion with OAuth2.
   */
  UPROPERTY(Config, EditAnywhere)
  FString CesiumIonAccessToken;
};
