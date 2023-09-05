// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#if WITH_EDITOR

#include "CesiumCameraManager.h"
#include "Editor.h"

class UWorld;
class ACesiumGeoreference;
class AGlobeAwareDefaultPawn;
class ACesium3DTileset;

namespace Cesium {

struct SceneGenerationContext {
  UWorld* world;
  ACesiumGeoreference* georeference;
  ACesiumCameraManager* cameraManager;
  AGlobeAwareDefaultPawn* pawn;
  std::vector<ACesium3DTileset*> tilesets;

  void setCamera(const FCesiumCamera& camera);
  void refreshTilesets();
  void setSuspendUpdate(bool suspend);
  bool areTilesetsDoneLoading();

  void trackForPlay();
  void initForPlay(SceneGenerationContext& creationContext);

  static FString testIonToken;
};

void createCommonWorldObjects(SceneGenerationContext& context);

void setupForDenver(SceneGenerationContext& context);
void setupForGoogleTiles(SceneGenerationContext& context);
void setupForMelbourne(SceneGenerationContext& context);

}; // namespace Cesium

#endif // #if WITH_EDITOR
