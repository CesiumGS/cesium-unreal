// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class ACesium3DTileset;
class UCesiumRasterOverlay;

namespace CesiumAsync {
class AsyncSystem;
class IAssetAccessor;
} // namespace CesiumAsync

DECLARE_LOG_CATEGORY_EXTERN(LogCesium, Log, All);

class FCesiumRuntimeModule : public IModuleInterface {
public:
  /** IModuleInterface implementation */
  virtual void StartupModule() override;
  virtual void ShutdownModule() override;
};

/**
 * The delegate for the OnCesium3DTilesetIonTroubleshooting, which is triggered
 * when the tileset encounters a load error.
 */
DECLARE_MULTICAST_DELEGATE_OneParam(
    FCesium3DTilesetIonTroubleshooting,
    ACesium3DTileset*);

CESIUMRUNTIME_API extern FCesium3DTilesetIonTroubleshooting
    OnCesium3DTilesetIonTroubleshooting;

/**
 * The delegate for the OnCesiumRasterOverlayIonTroubleshooting, which is
 * triggered when the tileset encounters a load error.
 */
DECLARE_MULTICAST_DELEGATE_OneParam(
    FCesiumRasterOverlayIonTroubleshooting,
    UCesiumRasterOverlay*);

CESIUMRUNTIME_API extern FCesiumRasterOverlayIonTroubleshooting
    OnCesiumRasterOverlayIonTroubleshooting;

CESIUMRUNTIME_API CesiumAsync::AsyncSystem& getAsyncSystem() noexcept;
CESIUMRUNTIME_API const std::shared_ptr<CesiumAsync::IAssetAccessor>&
getAssetAccessor();
