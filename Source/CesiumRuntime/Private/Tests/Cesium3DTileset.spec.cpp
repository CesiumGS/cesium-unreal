// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "Cesium3DTileset.h"
#include "CesiumTestHelpers.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

BEGIN_DEFINE_SPEC(
    FCesium3DTilesetSpec,
    "Cesium.Unit.3DTileset",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter);
END_DEFINE_SPEC(FCesium3DTilesetSpec);

void FCesium3DTilesetSpec::Define() {
  Describe("SharedImages", [this]() {
    It("should only load two textures", [this]() {
      UWorld* World = GEditor->PlayWorld;
      check(World);

      ACesium3DTileset* tileset = World->SpawnActor<ACesium3DTileset>();
      tileset->SetTilesetSource(ETilesetSource::FromUrl);
      tileset->SetUrl(FString::Printf(
          TEXT(
              "file:///%s/cesium-unreal/extern/cesium-native/Cesium3DTilesSelection/test/data/SharedImages"),
          *FPaths::ProjectPluginsDir()));
      tileset->SetActorLabel(TEXT("SharedImages"));
    });
  });
}
