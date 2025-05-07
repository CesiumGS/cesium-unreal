// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "Cesium3DTileset.h"
#include "CesiumGlobeAnchorComponent.h"
#include "CesiumGltfComponent.h"
#include "CesiumLoadTestCore.h"
#include "CesiumSceneGeneration.h"
#include "CesiumSunSky.h"
#include "CesiumTestHelpers.h"
#include "Engine/World.h"
#include "GlobeAwareDefaultPawn.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationTestSettings.h"
#include <Cesium3DTilesSelection/TilesetSharedAssetSystem.h>
#include <CesiumAsync/ICacheDatabase.h>

THIRD_PARTY_INCLUDES_START
#include <CesiumUtility/Uri.h>
THIRD_PARTY_INCLUDES_END

#define TEST_SCREEN_WIDTH 1280
#define TEST_SCREEN_HEIGHT 720

namespace Cesium {

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCesium3DTilesetSharedImages,
    "Cesium.Unit.3DTileset.SharedImages",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

static void setupForSharedImages(SceneGenerationContext& context) {
  context.setCommonProperties(
      FVector(21.16677692, -67.38013505, -6375355.1944),
      FVector(-12, -1300, -5),
      FRotator(0, 90, 0),
      60.0f);

  context.georeference->SetOriginEarthCenteredEarthFixed(FVector(0, 0, 0));
  context.pawn->SetActorLocation(FVector(485.0, 2400.0, 520.0));
  context.pawn->SetActorRotation(FQuat::MakeFromEuler(FVector(0, 0, 270)));

  context.sunSky->TimeZone = 9.0f;
  context.sunSky->UpdateSun();

  ACesiumGeoreference* georeference =
      context.world->SpawnActor<ACesiumGeoreference>();
  check(georeference != nullptr);
  georeference->SetOriginPlacement(EOriginPlacement::TrueOrigin);

  ACesium3DTileset* tileset = context.world->SpawnActor<ACesium3DTileset>();
  tileset->SetTilesetSource(ETilesetSource::FromCesiumIon);
  tileset->SetIonAssetID(2757071);
  tileset->SetIonAccessToken(SceneGenerationContext::testIonToken);

  tileset->SetActorLabel(TEXT("SharedImages"));
  tileset->SetGeoreference(georeference);
  tileset->SuspendUpdate = false;
  tileset->LogSelectionStats = true;
  context.tilesets.push_back(tileset);

  UCesiumGlobeAnchorComponent* GlobeAnchorComponent =
      NewObject<UCesiumGlobeAnchorComponent>(tileset, TEXT("GlobeAnchor"));
  tileset->AddInstanceComponent(GlobeAnchorComponent);
  GlobeAnchorComponent->SetAdjustOrientationForGlobeWhenMoving(false);
  GlobeAnchorComponent->SetGeoreference(georeference);
  GlobeAnchorComponent->RegisterComponent();
  GlobeAnchorComponent->MoveToEarthCenteredEarthFixedPosition(
      FVector(0.0, 0.0, 0.0));

  ADirectionalLight* Light = context.world->SpawnActor<ADirectionalLight>();
  Light->SetActorRotation(FQuat::MakeFromEuler(FVector(0, 0, 270)));
}

void tilesetPass(
    SceneGenerationContext& context,
    TestPass::TestingParameter parameter) {}

bool FCesium3DTilesetSharedImages::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(TestPass{"Refresh Pass", tilesetPass, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      setupForSharedImages,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCesium3DTilesetPhysicsWithSmallScale,
    "Cesium.Unit.3DTileset.PhysicsWithSmallScale",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

static void setupForPhysicsWithSmallScale(SceneGenerationContext& context) {
  context.setCommonProperties(
      FVector(21.16677692, -67.38013505, -6375355.1944),
      FVector(-12, -1300, -5),
      FRotator(0, 90, 0),
      60.0f);

  context.georeference->SetOriginEarthCenteredEarthFixed(FVector(0, 0, 0));
  context.pawn->SetActorLocation(FVector(485.0, 2400.0, 520.0));
  context.pawn->SetActorRotation(FQuat::MakeFromEuler(FVector(0, 0, 270)));

  context.sunSky->TimeZone = 9.0f;
  context.sunSky->UpdateSun();

  ACesiumGeoreference* georeference =
      context.world->SpawnActor<ACesiumGeoreference>();
  check(georeference != nullptr);
  georeference->SetOriginPlacement(EOriginPlacement::TrueOrigin);

  FString pluginContent =
      FPaths::ConvertRelativePathToFull(IPluginManager::Get()
                                            .FindPlugin(TEXT("CesiumForUnreal"))
                                            ->GetContentDir());
  FString tilesetPath = FPaths::Combine(
      pluginContent,
      TEXT("Tests"),
      TEXT("Tilesets"),
      TEXT("BigCube"),
      TEXT("tileset.json"));

  FString tilesetUriPath = UTF8_TO_TCHAR(
      CesiumUtility::Uri::nativePathToUriPath(TCHAR_TO_UTF8(*tilesetPath))
          .c_str());

  ACesium3DTileset* tileset = context.world->SpawnActor<ACesium3DTileset>();
  tileset->SetTilesetSource(ETilesetSource::FromUrl);
  tileset->SetUrl(TEXT("file://") + tilesetUriPath);
  tileset->SetActorScale3D(FVector(0.00001));

  tileset->SetActorLabel(TEXT("BigCubeSmallScale"));
  tileset->SetGeoreference(georeference);
  tileset->SuspendUpdate = false;
  tileset->LogSelectionStats = true;
  context.tilesets.push_back(tileset);

  UCesiumGlobeAnchorComponent* GlobeAnchorComponent =
      NewObject<UCesiumGlobeAnchorComponent>(tileset, TEXT("GlobeAnchor"));
  tileset->AddInstanceComponent(GlobeAnchorComponent);
  GlobeAnchorComponent->SetAdjustOrientationForGlobeWhenMoving(false);
  GlobeAnchorComponent->SetGeoreference(georeference);
  GlobeAnchorComponent->RegisterComponent();
  GlobeAnchorComponent->MoveToEarthCenteredEarthFixedPosition(
      FVector(0.0, 0.0, 0.0));

  ADirectionalLight* Light = context.world->SpawnActor<ADirectionalLight>();
  Light->SetActorRotation(FQuat::MakeFromEuler(FVector(0, 0, 270)));
}

bool checkPhysics(
    SceneGenerationContext& creationContext,
    SceneGenerationContext& playContext,
    TestPass::TestingParameter parameter) {
  FHitResult hit;

  FVector lookDirection =
      playContext.pawn->GetViewRotation().RotateVector(FVector::XAxisVector);

  return UKismetSystemLibrary::LineTraceSingle(
      playContext.world,
      playContext.pawn->GetPawnViewLocation(),
      playContext.pawn->GetPawnViewLocation() + lookDirection * 100000.0,
      ETraceTypeQuery::TraceTypeQuery1,
      true,
      {},
      EDrawDebugTrace::Persistent,
      hit,
      true);
}

bool FCesium3DTilesetPhysicsWithSmallScale::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(TestPass{"Refresh Pass", tilesetPass, checkPhysics});

  return RunLoadTest(
      GetBeautifiedTestName(),
      setupForPhysicsWithSmallScale,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

} // namespace Cesium

#endif
