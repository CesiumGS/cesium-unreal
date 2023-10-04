// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#if WITH_EDITOR

#include <functional>

#include "CesiumSceneGeneration.h"

namespace Cesium {

struct TestPass {
  FString name;
  std::function<void(SceneGenerationContext&)> setupStep;
  std::function<void(SceneGenerationContext&)> verifyStep;
};

bool RunLoadTest(
    const FString& testName,
    std::function<void(SceneGenerationContext&)> locationSetup,
    const std::vector<TestPass>& testPasses);

}; // namespace Cesium

#endif
