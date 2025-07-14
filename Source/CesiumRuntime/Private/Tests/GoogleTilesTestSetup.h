#pragma once

#if WITH_EDITOR

#include "CesiumSceneGeneration.h"
#include "CesiumTestPass.h"

namespace Cesium {

struct GoogleTilesTestSetup {
  static void setupRefreshTilesets(
      SceneGenerationContext& context,
      TestPass::TestingParameter parameter);

  static void setupClearCache(
      SceneGenerationContext& creationContext,
      TestPass::TestingParameter parameter);

  static void setupForLocation(
      SceneGenerationContext& context,
      const FVector& location,
      const FRotator& rotation,
      const FString& name);

  static void setupForPompidou(SceneGenerationContext& context);
  static void setupForChrysler(SceneGenerationContext& context);
  static void setupForGuggenheim(SceneGenerationContext& context);
  static void setupForDeathValley(SceneGenerationContext& context);
  static void setupForTokyo(SceneGenerationContext& context);
  static void setupForGoogleplex(SceneGenerationContext& context);
};

} // namespace Cesium

#endif
