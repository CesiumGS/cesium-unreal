// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumRuntime.h"
#include "CesiumRuntimeSettings.h"
#include "HAL/FileManager.h"
#include "HttpModule.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#include "SpdlogUnrealLoggerSink.h"
#include "UnrealAssetAccessor.h"
#include "UnrealTaskProcessor.h"

#include <Cesium3DTilesContent/registerAllTileContentTypes.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/CachingAssetAccessor.h>
#include <CesiumAsync/GunzipAssetAccessor.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/SqliteCache.h>
#include <CesiumUtility/Tracing.h>
#include <Modules/ModuleManager.h>
#include <spdlog/spdlog.h>

#if CESIUM_TRACING_ENABLED
#include <chrono>
#endif

#define LOCTEXT_NAMESPACE "FCesiumRuntimeModule"

DEFINE_LOG_CATEGORY(LogCesium);

void FCesiumRuntimeModule::StartupModule() {
  Cesium3DTilesContent::registerAllTileContentTypes();

  std::shared_ptr<spdlog::logger> pLogger = spdlog::default_logger();
  pLogger->sinks() = {std::make_shared<SpdlogUnrealLoggerSink>()};

  FModuleManager::Get().LoadModuleChecked(TEXT("HTTP"));

  CESIUM_TRACE_INIT(
      "cesium-trace-" +
      std::to_string(std::chrono::time_point_cast<std::chrono::microseconds>(
                         std::chrono::steady_clock::now())
                         .time_since_epoch()
                         .count()) +
      ".json");

  FString PluginShaderDir = FPaths::Combine(
      IPluginManager::Get().FindPlugin(TEXT("CesiumForUnreal"))->GetBaseDir(),
      TEXT("Shaders"));
  AddShaderSourceDirectoryMapping(
      TEXT("/Plugin/CesiumForUnreal"),
      PluginShaderDir);
}

void FCesiumRuntimeModule::ShutdownModule() { CESIUM_TRACE_SHUTDOWN(); }

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCesiumRuntimeModule, CesiumRuntime)

FCesium3DTilesetIonTroubleshooting OnCesium3DTilesetIonTroubleshooting{};
FCesiumRasterOverlayIonTroubleshooting
    OnCesiumRasterOverlayIonTroubleshooting{};
FCesiumFeaturesMetadataViewProperties OnCesiumFeaturesMetadataViewProperties{};

CesiumAsync::AsyncSystem& getAsyncSystem() noexcept {
  static CesiumAsync::AsyncSystem asyncSystem(
      std::make_shared<UnrealTaskProcessor>());
  return asyncSystem;
}

namespace {

std::string getCacheDatabaseName() {
#if PLATFORM_ANDROID
  FString BaseDirectory = FPaths::ProjectPersistentDownloadDir();
#elif PLATFORM_IOS
  FString BaseDirectory =
      FPaths::Combine(*FPaths::ProjectSavedDir(), TEXT("Cesium"));
  if (!IFileManager::Get().DirectoryExists(*BaseDirectory)) {
    IFileManager::Get().MakeDirectory(*BaseDirectory, true);
  }
#else
  FString BaseDirectory = FPaths::ProjectUserDir();
#endif

  FString CesiumDBFile =
      FPaths::Combine(*BaseDirectory, TEXT("cesium-request-cache.sqlite"));
  FString PlatformAbsolutePath =
      IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(
          *CesiumDBFile);

  UE_LOG(
      LogCesium,
      Display,
      TEXT("Caching Cesium requests in %s"),
      *PlatformAbsolutePath);

  return TCHAR_TO_UTF8(*PlatformAbsolutePath);
}

} // namespace

std::shared_ptr<CesiumAsync::ICacheDatabase>& getCacheDatabase() {
  static int MaxCacheItems =
      GetDefault<UCesiumRuntimeSettings>()->MaxCacheItems;

  static std::shared_ptr<CesiumAsync::ICacheDatabase> pCacheDatabase =
      std::make_shared<CesiumAsync::SqliteCache>(
          spdlog::default_logger(),
          getCacheDatabaseName(),
          MaxCacheItems);

  return pCacheDatabase;
}

const std::shared_ptr<CesiumAsync::IAssetAccessor>& getAssetAccessor() {
  static int RequestsPerCachePrune =
      GetDefault<UCesiumRuntimeSettings>()->RequestsPerCachePrune;
  static std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor =
      std::make_shared<CesiumAsync::GunzipAssetAccessor>(
          std::make_shared<CesiumAsync::CachingAssetAccessor>(
              spdlog::default_logger(),
              std::make_shared<UnrealAssetAccessor>(),
              getCacheDatabase(),
              RequestsPerCachePrune));
  return pAssetAccessor;
}
