// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include <functional>
#include <variant>

#include "Tests/AutomationCommon.h"

namespace Cesium {

struct SceneGenerationContext;

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

  bool isFastest = false;
};

} // namespace Cesium
