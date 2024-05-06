#pragma once

#if WITH_EDITOR

#include <variant>
#include <functional>

#include "CesiumSceneGeneration.h"
#include "Tests/AutomationCommon.h"


namespace Cesium {

struct TestPass {
  typedef std::variant<int, float> TestingParameter;
  typedef std::function<void(SceneGenerationContext &, TestingParameter)>
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

}

#endif
