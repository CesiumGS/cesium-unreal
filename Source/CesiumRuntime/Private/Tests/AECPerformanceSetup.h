#pragma once

#if WITH_EDITOR

#include "CesiumSceneGeneration.h"
#include "CesiumTestPass.h"

namespace Cesium {

struct AECPerformanceTestSetup {
  static const FString baseUrl;

  static const TMap<int32, FString> variants;

  static void setupRefreshTilesets(
      SceneGenerationContext& context,
      TestPass::TestingParameter parameter);

  static void setupClearCache(
      SceneGenerationContext& creationContext,
      TestPass::TestingParameter parameter);

  static void setupForLocation(
      SceneGenerationContext& context,
      const FVector& georeferenceOrigin,
      const FRotator& cameraRotation,
      const FString& name,
      int32 variant);

  static void setupForTilesetA0(SceneGenerationContext& context);
  static void setupForTilesetA1(SceneGenerationContext& context);

  static void setupForTilesetB0(SceneGenerationContext& context);
  static void setupForTilesetB1(SceneGenerationContext& context);

  static void setupForTilesetC0(SceneGenerationContext& context);
  static void setupForTilesetC1(SceneGenerationContext& context);

  static void setupForTilesetD0(SceneGenerationContext& context);
  static void setupForTilesetD1(SceneGenerationContext& context);

  static void setupForTilesetE0(SceneGenerationContext& context);
  static void setupForTilesetE1(SceneGenerationContext& context);

  static void setupForTilesetF0(SceneGenerationContext& context);
  static void setupForTilesetF1(SceneGenerationContext& context);

  static void setupForTilesetG0(SceneGenerationContext& context);
  static void setupForTilesetG1(SceneGenerationContext& context);

  static void setupForTilesetH0(SceneGenerationContext& context);
  static void setupForTilesetH1(SceneGenerationContext& context);

  static void setupForTilesetI0(SceneGenerationContext& context);
  static void setupForTilesetI1(SceneGenerationContext& context);

  static void setupForTilesetJ0(SceneGenerationContext& context);
  static void setupForTilesetJ1(SceneGenerationContext& context);

  static void setupForTilesetK0(SceneGenerationContext& context);
  static void setupForTilesetK1(SceneGenerationContext& context);

  static void setupForTilesetL0(SceneGenerationContext& context);
  static void setupForTilesetL1(SceneGenerationContext& context);

  static void setupForTilesetM0(SceneGenerationContext& context);
  static void setupForTilesetM1(SceneGenerationContext& context);

  static void setupForTilesetN0(SceneGenerationContext& context);
  static void setupForTilesetN1(SceneGenerationContext& context);

  static void setupForTilesetO0(SceneGenerationContext& context);
  static void setupForTilesetO1(SceneGenerationContext& context);

  static void setupForTilesetP0(SceneGenerationContext& context);
  static void setupForTilesetP1(SceneGenerationContext& context);

  static void setupForTilesetQ0(SceneGenerationContext& context);
  static void setupForTilesetQ1(SceneGenerationContext& context);

  static void setupForTilesetR0(SceneGenerationContext& context);
  static void setupForTilesetR1(SceneGenerationContext& context);
};

} // namespace Cesium

#endif
