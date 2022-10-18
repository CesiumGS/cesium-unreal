// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumRuntime.h"
#include "Cesium3DTilesSelection/registerAllTileContentTypes.h"
#include "CesiumAsync/CachingAssetAccessor.h"
#include "CesiumAsync/SqliteCache.h"
#include "CesiumUtility/Tracing.h"
#include "HAL/FileManager.h"
#include "HttpModule.h"
#include "Misc/Paths.h"
#include "SpdlogUnrealLoggerSink.h"
#include "UnrealAssetAccessor.h"
#include "UnrealTaskProcessor.h"
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <Modules/ModuleManager.h>
#include <spdlog/spdlog.h>

#if CESIUM_TRACING_ENABLED
#include <chrono>
#endif

#define LOCTEXT_NAMESPACE "FCesiumRuntimeModule"

DEFINE_LOG_CATEGORY(LogCesium);

void FCesiumRuntimeModule::StartupModule() {
  Cesium3DTilesSelection::registerAllTileContentTypes();

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
}

void FCesiumRuntimeModule::ShutdownModule() { CESIUM_TRACE_SHUTDOWN(); }

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCesiumRuntimeModule, CesiumRuntime)

FCesium3DTilesetIonTroubleshooting OnCesium3DTilesetIonTroubleshooting{};
FCesiumRasterOverlayIonTroubleshooting
    OnCesiumRasterOverlayIonTroubleshooting{};

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
  FString BaseDirectory = FPaths::EngineUserDir();
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

const std::shared_ptr<CesiumAsync::IAssetAccessor>& getAssetAccessor() {
  static std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor =
      std::make_shared<CesiumAsync::CachingAssetAccessor>(
          spdlog::default_logger(),
          std::make_shared<UnrealAssetAccessor>(),
          std::make_shared<CesiumAsync::SqliteCache>(
              spdlog::default_logger(),
              getCacheDatabaseName()));
  return pAssetAccessor;
}
