// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumRuntime.h"
#include "Cesium3DTiles/registerAllTileContentTypes.h"
#include "SpdlogUnrealLoggerSink.h"
#include <Modules/ModuleManager.h>
#include <spdlog/spdlog.h>

#define LOCTEXT_NAMESPACE "FCesiumRuntimeModule"

DEFINE_LOG_CATEGORY(LogCesium);

void FCesiumRuntimeModule::StartupModule() {
  Cesium3DTiles::registerAllTileContentTypes();

  std::shared_ptr<spdlog::logger> pLogger = spdlog::default_logger();
  pLogger->sinks() = {std::make_shared<SpdlogUnrealLoggerSink>()};

  FModuleManager::Get().LoadModuleChecked(TEXT("HTTP"));
}

void FCesiumRuntimeModule::ShutdownModule() {}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCesiumRuntimeModule, CesiumRuntime)