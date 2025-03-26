// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#if WITH_EDITOR

#include <functional>
#include <swl/variant.hpp>

#include "CesiumSceneGeneration.h"
#include "CesiumTestPass.h"
#include "Tests/AutomationCommon.h"

namespace Cesium {
typedef std::function<void(const std::vector<TestPass>&)> ReportCallback;

struct LoadTestContext {
  FString testName;
  std::vector<TestPass> testPasses;

  SceneGenerationContext creationContext;
  SceneGenerationContext playContext;

  float cameraFieldOfView = 90.0f;

  ReportCallback reportStep;

  void reset();
};

bool RunLoadTest(
    const FString& testName,
    std::function<void(SceneGenerationContext&)> locationSetup,
    const std::vector<TestPass>& testPasses,
    int viewportWidth,
    int viewportHeight,
    ReportCallback optionalReportStep = nullptr);

DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(
    TimeLoadingCommand,
    FString,
    loggingName,
    SceneGenerationContext&,
    creationContext,
    SceneGenerationContext&,
    playContext,
    TestPass&,
    pass);

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(
    LoadTestScreenshotCommand,
    FString,
    screenshotName);

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(
    TestCleanupCommand,
    LoadTestContext&,
    context);

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(
    InitForPlayWhenReady,
    SceneGenerationContext&,
    creationContext,
    SceneGenerationContext&,
    playContext);

}; // namespace Cesium

#endif
