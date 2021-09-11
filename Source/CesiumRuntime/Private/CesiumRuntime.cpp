// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumRuntime.h"
#include "Cesium3DTilesSelection/registerAllTileContentTypes.h"
#include "CesiumGeoreference.h"
#include "CesiumGeoreferenceCustomization.h"
#include "CesiumUtility/Tracing.h"
#include "PropertyEditorModule.h"
#include "SpdlogUnrealLoggerSink.h"
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

  // Register detail customization for CesiumGeoreference
  {
    auto& PropertyModule =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>(
            "PropertyEditor");

    PropertyModule.RegisterCustomClassLayout(
        ACesiumGeoreference::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(
            &FCesiumGeoreferenceCustomization::MakeInstance));

    PropertyModule.NotifyCustomizationModuleChanged();
  }

  CESIUM_TRACE_INIT(
      "cesium-trace-" +
      std::to_string(std::chrono::time_point_cast<std::chrono::microseconds>(
                         std::chrono::steady_clock::now())
                         .time_since_epoch()
                         .count()) +
      ".json");
}

void FCesiumRuntimeModule::ShutdownModule() {

  // Unregister the detail customization for CesiumGeoreference
  if (FModuleManager::Get().IsModuleLoaded("PropertyEditor")) {
    auto& PropertyModule =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>(
            "PropertyEditor");
    PropertyModule.UnregisterCustomClassLayout(
        ACesiumGeoreference::StaticClass()->GetFName());
  }

  CESIUM_TRACE_SHUTDOWN();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCesiumRuntimeModule, CesiumRuntime)
