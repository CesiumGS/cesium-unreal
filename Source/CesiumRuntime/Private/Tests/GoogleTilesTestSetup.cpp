#if WITH_EDITOR

#include "GoogleTilesTestSetup.h"

#include "Cesium3DTileset.h"
#include "CesiumAsync/ICacheDatabase.h"
#include "CesiumLoadTestCore.h"
#include "CesiumRuntime.h"
#include "CesiumSunSky.h"

namespace Cesium {

inline void GoogleTilesTestSetup::setupRefreshTilesets(
    SceneGenerationContext& context,
    TestPass::TestingParameter parameter) {
  context.refreshTilesets();
}

inline void GoogleTilesTestSetup::setupClearCache(
    SceneGenerationContext& context,
    TestPass::TestingParameter parameter) {
  std::shared_ptr<CesiumAsync::ICacheDatabase> pCacheDatabase =
      getCacheDatabase();
  pCacheDatabase->clearAll();
}

inline void GoogleTilesTestSetup::setupForLocation(
    SceneGenerationContext& context,
    const FVector& location,
    const FRotator& rotation,
    const FString& name) {
  context.setCommonProperties(location, FVector::ZeroVector, rotation, 60.0f);

  ACesium3DTileset* tileset = context.world->SpawnActor<ACesium3DTileset>();
  tileset->SetTilesetSource(ETilesetSource::FromCesiumIon);
  tileset->SetIonAssetID(2275207);
  tileset->SetIonAccessToken(SceneGenerationContext::testIonToken);
  tileset->SetActorLabel(name);
  context.tilesets.push_back(tileset);
}

inline void
GoogleTilesTestSetup::setupForPompidou(SceneGenerationContext& context) {
  setupForLocation(
      context,
      FVector(2.352200, 48.860600, 200),
      FRotator(-20.0, -90.0, 0.0),
      TEXT("Center Pompidou, Paris, France"));

  context.sunSky->TimeZone = 2.0f;
  context.sunSky->UpdateSun();
}

inline void
GoogleTilesTestSetup::setupForChrysler(SceneGenerationContext& context) {
  setupForLocation(
      context,
      FVector(-73.9752624659, 40.74697185903, 307.38),
      FRotator(-15.0, -90.0, 0.0),
      TEXT("Chrysler Building, NYC"));

  context.sunSky->TimeZone = -4.0f;
  context.sunSky->UpdateSun();
}

inline void
GoogleTilesTestSetup::setupForGuggenheim(SceneGenerationContext& context) {
  setupForLocation(
      context,
      FVector(-2.937, 43.2685, 150),
      FRotator(-15.0, 0.0, 0.0),
      TEXT("Guggenheim Museum, Bilbao, Spain"));

  context.sunSky->TimeZone = 2.0f;
  context.sunSky->UpdateSun();
}

inline void
GoogleTilesTestSetup::setupForDeathValley(SceneGenerationContext& context) {
  setupForLocation(
      context,
      FVector(-116.812278, 36.42, 300),
      FRotator(0, 0.0, 0.0),
      TEXT("Zabriskie Point, Death Valley National Park, California"));

  context.sunSky->TimeZone = -7.0f;
  context.sunSky->UpdateSun();
}

inline void
GoogleTilesTestSetup::setupForTokyo(SceneGenerationContext& context) {
  setupForLocation(
      context,
      FVector(139.7563178458, 35.652798383944, 525.62),
      FRotator(-15, -150, 0.0),
      TEXT("Tokyo Tower, Tokyo, Japan"));

  context.sunSky->TimeZone = 9.0f;
  context.sunSky->UpdateSun();
}

inline void
GoogleTilesTestSetup::setupForGoogleplex(SceneGenerationContext& context) {
  setupForLocation(
      context,
      FVector(-122.083969, 37.424492, 142.859116),
      FRotator(-25, 95, 0),
      TEXT("Google Photorealistic 3D Tiles"));
}

} // namespace Cesium

#endif
