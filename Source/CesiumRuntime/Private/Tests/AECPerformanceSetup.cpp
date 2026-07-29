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
      FVector(0.000075, -0.000067, -14.885801),
      FRotator(-9.865022, -116.400058, 0),
      TEXT("A"),
      0);

  context.sunSky->TimeZone = 0.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetA1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(0.000075, -0.000067, -14.885801),
      FRotator(-9.865022, -116.400058, 0),
      TEXT("A"),
      1);

  context.sunSky->TimeZone = 0.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetB0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(153.077527, -30.283263, 205.774323),
      FRotator(-30.867372, -51.822038, 0),
      TEXT("B"),
      0);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetB1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(153.077527, -30.283263, 205.774323),
      FRotator(-30.867372, -51.822038, 0),
      TEXT("B"),
      1);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetC0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(153.081, -30.308164, 44.407408),
      FRotator(-28.649103, 126.366824, 0),
      TEXT("C"),
      0);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetC1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(153.081, -30.308164, 44.407408),
      FRotator(-28.649103, 126.366824, 0),
      TEXT("C"),
      1);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetD0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(103.647192, 1.285037, 20.879465),
      FRotator(-8.486185, 139.006219, 0),
      TEXT("D"),
      0);

  context.sunSky->TimeZone = 7.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetD1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(103.647192, 1.285037, 20.879465),
      FRotator(-8.486185, 139.006219, 0),
      TEXT("D"),
      1);

  context.sunSky->TimeZone = 7.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetE0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(-111.647076, 40.144028, 23.997996),
      FRotator(-24.890024, -36.079948, 0),
      TEXT("E"),
      0);

  context.sunSky->TimeZone = -7.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetE1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(-111.647076, 40.144028, 23.997996),
      FRotator(-24.890024, -36.079948, 0),
      TEXT("E"),
      1);

  context.sunSky->TimeZone = -7.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetF0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(-89.298164, 34.946326, 117.802649),
      FRotator(-19.239172, 0.609666, 0),
      TEXT("F"),
      0);

  context.sunSky->TimeZone = -6.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetF1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(-89.298164, 34.946326, 117.802649),
      FRotator(-19.239172, 0.609666, 0),
      TEXT("F"),
      1);

  context.sunSky->TimeZone = -6.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetG0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(-104.821185, 41.022095, 2310.360097),
      FRotator(-79.844101, 1.209228, 0),
      TEXT("G"),
      0);

  context.sunSky->TimeZone = -7.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetG1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(-104.821185, 41.022095, 2310.360097),
      FRotator(-79.844101, 1.209228, 0),
      TEXT("G"),
      1);

  context.sunSky->TimeZone = -7.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetH0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(-0.002553, 0.001267, 279.320775),
      FRotator(-67.327954, 58.70202, 0),
      TEXT("H"),
      0);

  context.sunSky->TimeZone = 0.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetH1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(-0.002553, 0.001267, 279.320775),
      FRotator(-67.327954, 58.70202, 0),
      TEXT("H"),
      1);

  context.sunSky->TimeZone = 0.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetI0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(-0.002553, 0.001267, 279.320775),
      FRotator(-67.327954, 58.70202, 0),
      TEXT("I"),
      0);

  context.sunSky->TimeZone = 0.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetI1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(-0.002553, 0.001267, 279.320775),
      FRotator(-67.327954, 58.70202, 0),
      TEXT("I"),
      1);

  context.sunSky->TimeZone = 0.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetJ0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(145.081106, -37.779139, 42.869058),
      FRotator(-18.699018, -155.221659, 0),
      TEXT("J"),
      0);

  context.sunSky->TimeZone = 9.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetJ1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(145.081106, -37.779139, 42.869058),
      FRotator(-18.699018, -155.221659, 0),
      TEXT("J"),
      1);

  context.sunSky->TimeZone = 9.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetK0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(151.177915, -33.932124, 42.869058),
      FRotator(-20.959633, -11.467612, 0),
      TEXT("K"),
      0);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetK1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(151.177915, -33.932124, 42.869058),
      FRotator(-20.959633, -11.467612, 0),
      TEXT("K"),
      1);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetL0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(0.009731, 0.003466, -874.879431),
      FRotator(-38.473705, 134.796517, 0),
      TEXT("L"),
      0);

  context.sunSky->TimeZone = 0.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetL1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(0.009731, 0.003466, -874.879431),
      FRotator(-38.473705, 134.796517, 0),
      TEXT("L"),
      1);

  context.sunSky->TimeZone = 0.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetM0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(153.052168, -27.553686, 103.558762),
      FRotator(-3.466836, -129.800314, 0),
      TEXT("M"),
      0);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetM1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(153.052168, -27.553686, 103.558762),
      FRotator(-3.466836, -129.800314, 0),
      TEXT("M"),
      1);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetN0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(151.177915, -33.932124, 42.869058),
      FRotator(-20.959633, -11.467612, 0),
      TEXT("N"),
      0);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetN1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(151.177915, -33.932124, 42.869058),
      FRotator(-20.959633, -11.467612, 0),
      TEXT("N"),
      1);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetO0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(151.177265, -33.932771, 27.996889),
      FRotator(-8.463267, -25.799784, 0),
      TEXT("O"),
      0);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetO1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(151.177265, -33.932771, 27.996889),
      FRotator(-8.463267, -25.799784, 0),
      TEXT("O"),
      1);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetP0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(151.093434, -33.813435, 49.323094),
      FRotator(-26.473443, 118.394261, 0),
      TEXT("P"),
      0);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetP1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(151.093434, -33.813435, 49.323094),
      FRotator(-26.473443, 118.394261, 0),
      TEXT("P"),
      1);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetQ0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(-87.934045, 28.573852, -1.148601),
      FRotator(12.000101, 165.278029, 0),
      TEXT("Q"),
      0);

  context.sunSky->TimeZone = -6.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetQ1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(-87.934045, 28.573852, -1.148601),
      FRotator(12.000101, 165.278029, 0),
      TEXT("Q"),
      1);

  context.sunSky->TimeZone = -6.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetR0(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(151.093155, -33.814459, 25.695306),
      FRotator(-7.669473, -116.000782, 0),
      TEXT("R"),
      0);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

void AECPerformanceTestSetup::setupForTilesetR1(
    SceneGenerationContext& context) {

  setupForLocation(
      context,
      FVector(151.093155, -33.814459, 25.695306),
      FRotator(-7.669473, -116.000782, 0),
      TEXT("R"),
      1);

  context.sunSky->TimeZone = 10.0f;
  context.sunSky->UpdateSun();
}

} // namespace Cesium

#endif
