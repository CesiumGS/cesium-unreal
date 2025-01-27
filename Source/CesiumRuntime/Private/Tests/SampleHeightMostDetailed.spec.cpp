// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "Cesium3DTileset.h"
#include "CesiumSampleHeightMostDetailedAsyncAction.h"
#include "CesiumSceneGeneration.h"
#include "CesiumTestHelpers.h"
#include "Misc/AutomationTest.h"
#include "SampleHeightCallbackReceiver.h"

BEGIN_DEFINE_SPEC(
    FSampleHeightMostDetailedSpec,
    "Cesium.Unit.SampleHeightMostDetailed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::ServerContext |
        EAutomationTestFlags::CommandletContext |
        EAutomationTestFlags::ProductFilter)

TObjectPtr<ACesium3DTileset> pTileset;

END_DEFINE_SPEC(FSampleHeightMostDetailedSpec)

// The intention of these tests is not to verify that height querying produces
// correct heights, because the cesium-native tests already do that. It's just
// to verify that the Unreal wrapper API around cesium-native is working
// correctly.

void FSampleHeightMostDetailedSpec::Define() {
  Describe("Cesium World Terrain", [this]() {
    BeforeEach([this]() {
      CesiumTestHelpers::pushAllowTickInEditor();

      UWorld* pWorld = CesiumTestHelpers::getGlobalWorldContext();
      pTileset = pWorld->SpawnActor<ACesium3DTileset>();
      pTileset->SetIonAssetID(1);
#if WITH_EDITOR
      pTileset->SetIonAccessToken(Cesium::SceneGenerationContext::testIonToken);
      pTileset->SetActorLabel(TEXT("Cesium World Terrain"));
#endif
    });

    AfterEach(EAsyncExecution::TaskGraphMainThread, [this]() {
      pTileset->Destroy();

      CesiumTestHelpers::popAllowTickInEditor();
    });

    LatentIt(
        "works with an empty array of positions",
        EAsyncExecution::TaskGraphMainThread,
        [this](const FDoneDelegate& done) {
          pTileset->SampleHeightMostDetailed(
              {},
              FCesiumSampleHeightMostDetailedCallback::CreateLambda(
                  [this, done](
                      ACesium3DTileset* pTileset,
                      const TArray<FCesiumSampleHeightResult>& result,
                      const TArray<FString>& warnings) {
                    TestEqual("Number of results", result.Num(), 0);
                    TestEqual("Number of warnings", warnings.Num(), 0);
                    done.ExecuteIfBound();
                  }));
        });

    LatentIt(
        "works with a single position",
        EAsyncExecution::TaskGraphMainThread,
        [this](const FDoneDelegate& done) {
          pTileset->SampleHeightMostDetailed(
              {FVector(-105.1, 40.1, 1.0)},
              FCesiumSampleHeightMostDetailedCallback::CreateLambda(
                  [this, done](
                      ACesium3DTileset* pTileset,
                      const TArray<FCesiumSampleHeightResult>& result,
                      const TArray<FString>& warnings) {
                    TestEqual("Number of results", result.Num(), 1);
                    TestEqual("Number of warnings", warnings.Num(), 0);
                    TestTrue("SampleSuccess", result[0].SampleSuccess);
                    TestEqual(
                        "Longitude",
                        result[0].LongitudeLatitudeHeight.X,
                        -105.1,
                        1e-12);
                    TestEqual(
                        "Latitude",
                        result[0].LongitudeLatitudeHeight.Y,
                        40.1,
                        1e-12);
                    TestTrue(
                        "Height",
                        !FMath::IsNearlyEqual(
                            result[0].LongitudeLatitudeHeight.Z,
                            1.0,
                            1.0));
                    done.ExecuteIfBound();
                  }));
        });

    LatentIt(
        "works with multiple positions",
        EAsyncExecution::TaskGraphMainThread,
        [this](const FDoneDelegate& done) {
          pTileset->SampleHeightMostDetailed(
              {FVector(-105.1, 40.1, 1.0), FVector(105.1, -40.1, 1.0)},
              FCesiumSampleHeightMostDetailedCallback::CreateLambda(
                  [this, done](
                      ACesium3DTileset* pTileset,
                      const TArray<FCesiumSampleHeightResult>& result,
                      const TArray<FString>& warnings) {
                    TestEqual("Number of results", result.Num(), 2);
                    TestEqual("Number of warnings", warnings.Num(), 0);
                    TestTrue("SampleSuccess", result[0].SampleSuccess);
                    TestEqual(
                        "Longitude",
                        result[0].LongitudeLatitudeHeight.X,
                        -105.1,
                        1e-12);
                    TestEqual(
                        "Latitude",
                        result[0].LongitudeLatitudeHeight.Y,
                        40.1,
                        1e-12);
                    TestTrue(
                        "Height",
                        !FMath::IsNearlyEqual(
                            result[0].LongitudeLatitudeHeight.Z,
                            1.0,
                            1.0));
                    TestTrue("SampleSuccess", result[1].SampleSuccess);
                    TestEqual(
                        "Longitude",
                        result[1].LongitudeLatitudeHeight.X,
                        105.1,
                        1e-12);
                    TestEqual(
                        "Latitude",
                        result[1].LongitudeLatitudeHeight.Y,
                        -40.1,
                        1e-12);
                    TestTrue(
                        "Height",
                        !FMath::IsNearlyEqual(
                            result[1].LongitudeLatitudeHeight.Z,
                            1.0,
                            1.0));
                    done.ExecuteIfBound();
                  }));
        });
  });

  Describe("Melbourne Photogrammetry", [this]() {
    BeforeEach([this]() {
      CesiumTestHelpers::pushAllowTickInEditor();

      UWorld* pWorld = CesiumTestHelpers::getGlobalWorldContext();
      pTileset = pWorld->SpawnActor<ACesium3DTileset>();
      pTileset->SetIonAssetID(69380);
#if WITH_EDITOR
      pTileset->SetIonAccessToken(Cesium::SceneGenerationContext::testIonToken);
      pTileset->SetActorLabel(TEXT("Melbourne Photogrammetry"));
#endif
    });

    AfterEach(EAsyncExecution::TaskGraphMainThread, [this]() {
      pTileset->Destroy();

      CesiumTestHelpers::popAllowTickInEditor();
    });

    LatentIt(
        "indicates !HeightSampled for position outside tileset",
        EAsyncExecution::TaskGraphMainThread,
        [this](const FDoneDelegate& done) {
          pTileset->SampleHeightMostDetailed(
              // Somewhere in Sydney, not Melbourne
              {FVector(151.20972, -33.87100, 1.0)},
              FCesiumSampleHeightMostDetailedCallback::CreateLambda(
                  [this, done](
                      ACesium3DTileset* pTileset,
                      const TArray<FCesiumSampleHeightResult>& result,
                      const TArray<FString>& warnings) {
                    TestEqual("Number of results", result.Num(), 1);
                    TestEqual("Number of warnings", warnings.Num(), 0);
                    TestTrue("SampleSuccess", !result[0].SampleSuccess);
                    TestEqual(
                        "Longitude",
                        result[0].LongitudeLatitudeHeight.X,
                        151.20972,
                        1e-12);
                    TestEqual(
                        "Latitude",
                        result[0].LongitudeLatitudeHeight.Y,
                        -33.87100,
                        1e-12);
                    TestEqual(
                        "Height",
                        result[0].LongitudeLatitudeHeight.Z,
                        1.0,
                        1e-12);
                    done.ExecuteIfBound();
                  }));
        });

    LatentIt(
        "can be queried via Blueprint interface",
        EAsyncExecution::TaskGraphMainThread,
        [this](const FDoneDelegate& done) {
          UCesiumSampleHeightMostDetailedAsyncAction* pAsync =
              UCesiumSampleHeightMostDetailedAsyncAction::
                  SampleHeightMostDetailed(
                      pTileset,
                      {FVector(144.93406, -37.82457, 1.0)});

          USampleHeightCallbackReceiver::Bind(
              pAsync->OnHeightsSampled,
              [this, done](
                  const TArray<FCesiumSampleHeightResult>& result,
                  const TArray<FString>& warnings) {
                TestEqual("Number of results", result.Num(), 1);
                TestEqual("Number of warnings", warnings.Num(), 0);
                TestTrue("SampleSuccess", result[0].SampleSuccess);
                TestEqual(
                    "Longitude",
                    result[0].LongitudeLatitudeHeight.X,
                    144.93406,
                    1e-12);
                TestEqual(
                    "Latitude",
                    result[0].LongitudeLatitudeHeight.Y,
                    -37.82457,
                    1e-12);
                TestTrue(
                    "Height",
                    !FMath::IsNearlyEqual(
                        result[0].LongitudeLatitudeHeight.Z,
                        1.0,
                        1.0));
                done.ExecuteIfBound();
              });

          pAsync->Activate();
        });
  });

  Describe("Two tilesets in rapid succession", [this]() {
    BeforeEach([this]() { CesiumTestHelpers::pushAllowTickInEditor(); });

    AfterEach(EAsyncExecution::TaskGraphMainThread, [this]() {
      CesiumTestHelpers::popAllowTickInEditor();
    });

    LatentIt(
        "",
        EAsyncExecution::TaskGraphMainThread,
        [this](const FDoneDelegate& done) {
          UWorld* pWorld = CesiumTestHelpers::getGlobalWorldContext();

          ACesium3DTileset* pTileset1 = pWorld->SpawnActor<ACesium3DTileset>();
          pTileset1->SetIonAssetID(1);
#if WITH_EDITOR
          pTileset1->SetIonAccessToken(
              Cesium::SceneGenerationContext::testIonToken);
#endif

          pTileset1->SampleHeightMostDetailed(
              {FVector(-105.1, 40.1, 1.0)},
              FCesiumSampleHeightMostDetailedCallback::CreateLambda(
                  [this, pWorld, done](
                      ACesium3DTileset* pTileset,
                      const TArray<FCesiumSampleHeightResult>& result,
                      const TArray<FString>& warnings) {
                    TestEqual("Number of results", result.Num(), 1);
                    TestEqual("Number of warnings", warnings.Num(), 0);
                    TestTrue("SampleSuccess", result[0].SampleSuccess);

                    ACesium3DTileset* pTileset2 =
                        pWorld->SpawnActor<ACesium3DTileset>();
                    pTileset2->SetIonAssetID(1);
#if WITH_EDITOR
                    pTileset2->SetIonAccessToken(
                        Cesium::SceneGenerationContext::testIonToken);
#endif
                    pTileset2->SampleHeightMostDetailed(
                        {FVector(105.1, 40.1, 1.0)},
                        FCesiumSampleHeightMostDetailedCallback::CreateLambda(
                            [this, pWorld, done](
                                ACesium3DTileset* pTileset,
                                const TArray<FCesiumSampleHeightResult>& result,
                                const TArray<FString>& warnings) {
                              TestEqual("Number of results", result.Num(), 1);
                              TestEqual(
                                  "Number of warnings",
                                  warnings.Num(),
                                  0);
                              TestTrue(
                                  "SampleSuccess",
                                  result[0].SampleSuccess);

                              done.ExecuteIfBound();
                            }));
                  }));
        });
  });

  Describe("Broken tileset", [this]() {
    BeforeEach([this]() { CesiumTestHelpers::pushAllowTickInEditor(); });

    AfterEach(EAsyncExecution::TaskGraphMainThread, [this]() {
      CesiumTestHelpers::popAllowTickInEditor();
    });

    LatentIt(
        "invalid tileset URL",
        EAsyncExecution::TaskGraphMainThread,
        [this](const FDoneDelegate& done) {
          // Two slightly different error messages will occur, depending on
          // whether there's a web server running on localhost.
          this->AddExpectedError(
              TEXT("(Errors when loading)|(error occurred)"));

          UWorld* pWorld = CesiumTestHelpers::getGlobalWorldContext();

          ACesium3DTileset* pTileset = pWorld->SpawnActor<ACesium3DTileset>();
          pTileset->SetTilesetSource(ETilesetSource::FromUrl);
          pTileset->SetUrl("http://localhost/notgonnawork");

          pTileset->SampleHeightMostDetailed(
              {FVector(-105.1, 40.1, 1.0)},
              FCesiumSampleHeightMostDetailedCallback::CreateLambda(
                  [this, done](
                      ACesium3DTileset* pTileset,
                      const TArray<FCesiumSampleHeightResult>& result,
                      const TArray<FString>& warnings) {
                    TestEqual("Number of results", result.Num(), 1);
                    TestEqual("Number of warnings", warnings.Num(), 1);
                    TestFalse("SampleSuccess", result[0].SampleSuccess);
                    TestEqual(
                        "Longitude",
                        result[0].LongitudeLatitudeHeight.X,
                        -105.1,
                        1e-12);
                    TestEqual(
                        "Latitude",
                        result[0].LongitudeLatitudeHeight.Y,
                        40.1,
                        1e-12);
                    TestEqual(
                        "Height",
                        result[0].LongitudeLatitudeHeight.Z,
                        1.0,
                        1e-12);
                    TestTrue(
                        "Error message",
                        warnings[0].Contains(TEXT("failed to load")));
                    done.ExecuteIfBound();
                  }));
        });

    LatentIt(
        "tileset parameter is nullptr",
        EAsyncExecution::TaskGraphMainThread,
        [this](const FDoneDelegate& done) {
          UCesiumSampleHeightMostDetailedAsyncAction* pAsync =
              UCesiumSampleHeightMostDetailedAsyncAction::
                  SampleHeightMostDetailed(
                      nullptr,
                      {FVector(144.93406, -37.82457, 1.0)});

          USampleHeightCallbackReceiver::Bind(
              pAsync->OnHeightsSampled,
              [this, done](
                  const TArray<FCesiumSampleHeightResult>& result,
                  const TArray<FString>& warnings) {
                TestEqual("Number of results", result.Num(), 0);
                TestEqual("Number of warnings", warnings.Num(), 1);
                done.ExecuteIfBound();
              });

          pAsync->Activate();
        });
  });


}
