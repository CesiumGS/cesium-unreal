#pragma once

#if WITH_EDITOR

#include <functional>
#include <variant>

#include "CesiumSceneGeneration.h"
#include "Tests/AutomationCommon.h"

namespace Cesium {

struct TestPass {
  typedef std::variant<int, float> TestingParameter;
  typedef std::function<void(SceneGenerationContext&, TestingParameter)>
      SetupPassCallback;
  typedef std::function<
      bool(SceneGenerationContext&, SceneGenerationContext&, TestingParameter)>
      VerifyPassCallback;

  FString name;
  SetupPassCallback setupStep;
  VerifyPassCallback verifyStep;
  TestingParameter optionalParameter;

  bool testInProgress = false;
  double startMark = 0;
  double endMark = 0;
  double elapsedTime = 0;

  bool isFastest = false;
};

} // namespace Cesium

#endif
