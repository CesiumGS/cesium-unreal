// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#if WITH_EDITOR

#include <functional>

#include "CesiumSceneGeneration.h"

namespace Cesium {

struct TestPass {
  typedef std::variant<int, float> TestingParameter;
  typedef std::function<void(SceneGenerationContext&, TestingParameter)>
      PassCallback;

  FString name;
  PassCallback setupStep;
  PassCallback verifyStep;
  TestingParameter optionalParameter;

  bool testInProgress = false;
  double startMark = 0;
  double endMark = 0;
  double elapsedTime = 0;
};

bool RunLoadTest(
    const FString& testName,
    std::function<void(SceneGenerationContext&)> locationSetup,
    const std::vector<TestPass>& testPasses,
    int viewportWidth,
    int viewportHeight);

}; // namespace Cesium

#endif
