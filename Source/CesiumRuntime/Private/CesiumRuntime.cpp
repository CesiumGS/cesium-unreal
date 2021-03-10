// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumRuntime.h"
#include "Cesium3DTiles/registerAllTileContentTypes.h"
#include "SpdlogUnrealLoggerSink.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <sstream>

#define LOCTEXT_NAMESPACE "FCesiumRuntimeModule"

class LStream : public std::stringbuf {
protected:
  int sync() {
    UE_LOG(LogTemp, Warning, TEXT("%s"), *FString(str().c_str()));
    str("");
    return std::stringbuf::sync();
  }
};

static LStream LogStream;

void FCesiumRuntimeModule::StartupModule() {
  // This code will execute after your module is loaded into memory; the exact
  // timing is specified in the .uplugin file per-module
  Cesium3DTiles::registerAllTileContentTypes();

  std::cout.rdbuf(&LogStream);
  std::cerr.rdbuf(&LogStream);

  std::shared_ptr<spdlog::logger> pLogger = spdlog::default_logger();
  pLogger->sinks() = {std::make_shared<SpdlogUnrealLoggerSink>()};
}

void FCesiumRuntimeModule::ShutdownModule() {
  // This function may be called during shutdown to clean up your module.  For
  // modules that support dynamic reloading, we call this function before
  // unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCesiumRuntimeModule, CesiumRuntime)