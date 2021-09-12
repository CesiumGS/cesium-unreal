// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumRuntime.h"
#include "Cesium3DTilesSelection/registerAllTileContentTypes.h"
#include "CesiumGeoreference.h"
#include "CesiumGeoreferenceCustomization.h"
#include "CesiumUtility/Tracing.h"
#include "SpdlogUnrealLoggerSink.h"
#include <Modules/ModuleManager.h>
#include <spdlog/spdlog.h>

#if WITH_EDITOR
#include "PropertyEditorModule.h"
#endif // WITH_EDITOR

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

#if WITH_EDITOR
  // Register detail customization for CesiumGeoreference
  FPropertyEditorModule& PropertyEditorModule =
      FModuleManager::LoadModuleChecked<FPropertyEditorModule>(
          "PropertyEditor");

  PropertyEditorModule.RegisterCustomClassLayout(
      ACesiumGeoreference::StaticClass()->GetFName(),
      FOnGetDetailCustomizationInstance::CreateStatic(
          &FCesiumGeoreferenceCustomization::MakeInstance));

  PropertyEditorModule.NotifyCustomizationModuleChanged();
#endif // WITH_EDITOR

  CESIUM_TRACE_INIT(
      "cesium-trace-" +
      std::to_string(std::chrono::time_point_cast<std::chrono::microseconds>(
                         std::chrono::steady_clock::now())
                         .time_since_epoch()
                         .count()) +
      ".json");
}

void FCesiumRuntimeModule::ShutdownModule() {

#if WITH_EDITOR
  // Unregister the detail customization for CesiumGeoreference
  if (FModuleManager::Get().IsModuleLoaded("PropertyEditor")) {
    FPropertyEditorModule& PropertyEditorModule =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>(
            "PropertyEditor");
    PropertyEditorModule.UnregisterCustomClassLayout(
        ACesiumGeoreference::StaticClass()->GetFName());
  }
#endif // WITH_EDITOR

  CESIUM_TRACE_SHUTDOWN();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCesiumRuntimeModule, CesiumRuntime)
