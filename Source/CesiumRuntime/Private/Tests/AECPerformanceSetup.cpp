#if WITH_EDITOR

#include "AECPerformanceSetup.h"

#include "Cesium3DTileset.h"
#include "CesiumAsync/ICacheDatabase.h"
#include "CesiumLoadTestCore.h"
#include "CesiumRuntime.h"
#include "CesiumSunSky.h"

namespace Cesium {

const FString AECPerformanceTestSetup::baseUrl = "file:///C:/Dev/performance/";

const TMap<int32, FString> AECPerformanceTestSetup::variants = {
    {0, "no-ext"},
    {1, "edges"}};

void AECPerformanceTestSetup::setupRefreshTilesets(
    SceneGenerationContext& context,
    TestPass::TestingParameter parameter) {
  context.refreshTilesets();
}

void AECPerformanceTestSetup::setupClearCache(
    SceneGenerationContext& context,
    TestPass::TestingParameter parameter) {
  std::shared_ptr<CesiumAsync::ICacheDatabase> pCacheDatabase =
      getCacheDatabase();
  pCacheDatabase->clearAll();
}

void AECPerformanceTestSetup::setupForLocation(
    SceneGenerationContext& context,
    const FVector& georeferenceOrigin,
    const FRotator& cameraRotation,
    const FString& name,
    int32 variant) {
  context.setCommonProperties(
      georeferenceOrigin,
      FVector::ZeroVector,
      cameraRotation,
      60.0f);

  FString url = baseUrl + name + "/" + variants[variant] + "/tileset.json";
  ACesium3DTileset* tileset = context.world->SpawnActor<ACesium3DTileset>();
  tileset->SetTilesetSource(ETilesetSource::FromUrl);
  tileset->SetUrl(url);
  tileset->SetActorLabel(name);
  context.tilesets.push_back(tileset);
}

void AECPerformanceTestSetup::setupForTilesetA0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(0.000287, 0.000289, 32.000162),
      FRotator(-35.264798, 135, 0),
      TEXT("A"),
      0);

  context.sunSky->TimeZone = 0.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetA1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(0.000287, 0.000289, 32.000162),
      FRotator(-35.264798, 135, 0),
      TEXT("A"),
      1);

  context.sunSky->TimeZone = 0.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetB0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(153.244486, -30.175477, 14270.878238),
      FRotator(-33.450034, 138.524321, 0),
      TEXT("B"),
      0);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetB1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(153.244486, -30.175477, 14270.878238),
      FRotator(-33.450034, 138.524321, 0),
      TEXT("B"),
      1);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetC0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(153.244486, -30.175477, 14270.878238),
      FRotator(-33.450034, 138.524321, 0),
      TEXT("C"),
      0);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetC1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(153.244486, -30.175477, 14270.878238),
      FRotator(-33.450034, 138.524321, 0),
      TEXT("C"),
      1);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetD0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(103.660255, 1.3027, 2049.861654),
      FRotator(-35.290489, 135.000478, 0),
      TEXT("D"),
      0);

  context.sunSky->TimeZone = 7.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetD1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(103.660255, 1.3027, 2049.861654),
      FRotator(-35.290489, 135.000478, 0),
      TEXT("D"),
      1);

  context.sunSky->TimeZone = 7.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetE0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(-111.518115, 40.234581, 8874.704235),
      FRotator(-33.599439, 135.279056, 0),
      TEXT("E"),
      0);

  context.sunSky->TimeZone = -7.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetE1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(-111.518115, 40.234581, 8874.704235),
      FRotator(-33.599439, 135.279056, 0),
      TEXT("E"),
      1);

  context.sunSky->TimeZone = -7.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetF0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(-89.28925, 34.95378, 936.496303),
      FRotator(-35.27493, 135.005217, 0),
      TEXT("F"),
      0);

  context.sunSky->TimeZone = -6.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetF1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(-89.28925, 34.95378, 936.496303),
      FRotator(-35.27493, 135.005217, 0),
      TEXT("F"),
      1);

  context.sunSky->TimeZone = -6.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetG0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(-104.768621, 41.05502, 4467.448873),
      FRotator(-27.22203, 135.998267, 0),
      TEXT("G"),
      0);

  context.sunSky->TimeZone = -7.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetG1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(-104.768621, 41.05502, 4467.448873),
      FRotator(-27.22203, 135.998267, 0),
      TEXT("G"),
      1);

  context.sunSky->TimeZone = -7.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetH0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(0.024799, 0.023635, 2729.545791),
      FRotator(-35.498595, 132.799711, 0),
      TEXT("H"),
      0);

  context.sunSky->TimeZone = 0.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetH1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(0.024799, 0.023635, 2729.545791),
      FRotator(-35.498595, 132.799711, 0),
      TEXT("H"),
      1);

  context.sunSky->TimeZone = 0.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetI0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(0.024799, 0.023635, 2729.545791),
      FRotator(-35.498595, 132.799711, 0),
      TEXT("I"),
      0);

  context.sunSky->TimeZone = 0.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetI1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(0.024799, 0.023635, 2729.545791),
      FRotator(-35.498595, 132.799711, 0),
      TEXT("I"),
      1);

  context.sunSky->TimeZone = 0.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetJ0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(145.052347, -37.799518, 2906.153212),
      FRotator(-44.324084, -30.737397, 0),
      TEXT("J"),
      0);

  context.sunSky->TimeZone = 9.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetJ1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(145.052347, -37.799518, 2906.153212),
      FRotator(-44.324084, -30.737397, 0),
      TEXT("J"),
      1);

  context.sunSky->TimeZone = 9.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetK0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(151.17909, -33.934336, 205.811273),
      FRotator(-31.855117, -89.983212, 0),
      TEXT("K"),
      0);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetK1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(151.17909, -33.934336, 205.811273),
      FRotator(-31.855117, -89.983212, 0),
      TEXT("K"),
      1);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetM0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(153.052995, -27.552007, 233.12071),
      FRotator(-35.266017, 134.999401, 0),
      TEXT("M"),
      0);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetM1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(153.052995, -27.552007, 233.12071),
      FRotator(-35.266017, 134.999401, 0),
      TEXT("M"),
      1);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetN0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(151.17909, -33.934336, 205.811273),
      FRotator(-31.855117, -89.983212, 0),
      TEXT("N"),
      0);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetN1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(151.17909, -33.934336, 205.811273),
      FRotator(-31.855117, -89.983212, 0),
      TEXT("N"),
      1);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetO0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(151.178281, -33.931789, 165.032743),
      FRotator(-43.506419, 118.707747, 0),
      TEXT("O"),
      0);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetO1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(151.178281, -33.931789, 165.032743),
      FRotator(-43.506419, 118.707747, 0),
      TEXT("O"),
      1);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetP0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(151.097874, -33.809544, 550.42273),
      FRotator(-35.270892, 134.996925, 0),
      TEXT("P"),
      0);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetP1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(151.097874, -33.809544, 550.42273),
      FRotator(-35.270892, 134.996925, 0),
      TEXT("P"),
      1);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetQ0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(-87.836437, 28.589484, 13754.033049),
      FRotator(-45.645534, -178.651315, 0),
      TEXT("Q"),
      0);

  context.sunSky->TimeZone = -6.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetQ1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(-87.836437, 28.589484, 13754.033049),
      FRotator(-45.645534, -178.651315, 0),
      TEXT("Q"),
      1);

  context.sunSky->TimeZone = -6.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetR0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(151.094597, -33.813012, 274.949843),
      FRotator(-49.466374, 140.998639, 0),
      TEXT("R"),
      0);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetR1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(151.094597, -33.813012, 274.949843),
      FRotator(-49.466374, 140.998639, 0),
      TEXT("R"),
      1);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

} // namespace Cesium

#endif
