// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#if WITH_EDITOR

#include <vector>

class UWorld;
class ACesiumGeoreference;
class AGlobeAwareDefaultPawn;
class ACesium3DTileset;
class ACesiumCameraManager;

namespace Cesium {

struct SceneGenerationContext {
  UWorld* world;
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
  bool areTilesetsDoneLoading();

  void trackForPlay();
  void initForPlay(SceneGenerationContext& creationContext);
  void syncWorldCamera();

  static FString testIonToken;
  static FString testGoogleUrl;
};

void createCommonWorldObjects(SceneGenerationContext& context);

void setupForDenver(SceneGenerationContext& context);
void setupForGoogleplex(SceneGenerationContext& context);
void setupForMontrealPointCloud(SceneGenerationContext& context);

}; // namespace Cesium

#endif // #if WITH_EDITOR
