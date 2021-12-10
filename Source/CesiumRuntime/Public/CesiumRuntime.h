// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCesium, Log, All);

class FCesiumRuntimeModule : public IModuleInterface {
public:
  /** IModuleInterface implementation */
  virtual void StartupModule() override;
  virtual void ShutdownModule() override;
};

/**
 * The delegate for the ACesium3DTileset::OnLoadError, which is triggered when
 * the tileset encounters a load error.
 */
DECLARE_MULTICAST_DELEGATE_OneParam(
    FCesium3DTilesetIonTroubleshooting,
    ACesium3DTileset*);

CESIUMRUNTIME_API extern FCesium3DTilesetIonTroubleshooting
    OnCesium3DTilesetIonTroubleshooting;
