// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include <vector>

class UWorld;
class ACesiumSunSky;
class ACesiumGeoreference;
class AGlobeAwareDefaultPawn;
class ACesium3DTileset;
class ACesiumCameraManager;

namespace Cesium {

struct SceneGenerationContext {
  UWorld* world;
  ACesiumSunSky* sunSky;
  ACesiumGeoreference* georeference;
  AGlobeAwareDefaultPawn* pawn;
  std::vector<ACesium3DTileset*> tilesets;

  FVector startPosition;
  FRotator startRotation;
  float startFieldOfView;

  void setCommonProperties(
      const FVector& origin,
      const FVector& position,
      const FRotator& rotation,
      float fieldOfView);

  void refreshTilesets();
  void setSuspendUpdate(bool suspend);
  void setMaximumSimultaneousTileLoads(int32 value);
  bool areTilesetsDoneLoading();

  void trackForPlay();
  void initForPlay(SceneGenerationContext& creationContext);
  void syncWorldCamera();

  static FString testIonToken;
};

void createCommonWorldObjects(SceneGenerationContext& context);

}; // namespace Cesium

#endif // #if WITH_EDITOR
