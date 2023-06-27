// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "CesiumGlobeAnchorComponent.h"
#include "CesiumTestHelpers.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GlobeAwareDefaultPawn.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"

BEGIN_DEFINE_SPEC(
    FGlobeAwareDefaultPawnSpec,
    "Cesium.GlobeAwareDefaultPawn",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)

FDelegateHandle subscriptionPostPIEStarted;

END_DEFINE_SPEC(FGlobeAwareDefaultPawnSpec)

void FGlobeAwareDefaultPawnSpec::Define() {
  Describe(
      "should not spike altitude when very close to final destination",
      [this]() {
        LatentBeforeEach(
            EAsyncExecution::TaskGraphMainThread,
            [this](const FDoneDelegate& done) {
              UWorld* pWorld = FAutomationEditorCommonUtils::CreateNewMap();

              TSoftObjectPtr<UObject> DynamicPawn =
                  TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT(
                      "Class'/CesiumForUnreal/DynamicPawn.DynamicPawn_C'")));
              AGlobeAwareDefaultPawn* pPawn =
                  pWorld->SpawnActor<AGlobeAwareDefaultPawn>(
                      Cast<UClass>(DynamicPawn.LoadSynchronous()));

              subscriptionPostPIEStarted =
                  FEditorDelegates::PostPIEStarted.AddLambda(
                      [done](bool isSimulating) { done.Execute(); });
              FRequestPlaySessionParams params{};
              GEditor->RequestPlaySession(params);
            });
        BeforeEach(EAsyncExecution::TaskGraphMainThread, [this]() {
          FEditorDelegates::PostPIEStarted.Remove(subscriptionPostPIEStarted);
        });
        AfterEach(EAsyncExecution::TaskGraphMainThread, [this]() {
          GEditor->RequestEndPlayMap();
        });
        It("", [this]() {
          UWorld* pWorld = GEditor->PlayWorld;

          TActorIterator<AGlobeAwareDefaultPawn> it(pWorld);
          AGlobeAwareDefaultPawn* pPawn = *it;
          pPawn->FlyToDuration = 5.0f;
          UCesiumGlobeAnchorComponent* pGlobeAnchor =
              pPawn->FindComponentByClass<UCesiumGlobeAnchorComponent>();
          TestNotNull("pGlobeAnchor", pGlobeAnchor);

          // Start flying somewhere else
          pPawn->FlyToLocationLongitudeLatitudeHeight(
              FVector(25.0, 10.0, 100.0),
              0.0,
              0.0,
              false);

          // Tick almost to the end
          pPawn->Tick(4.9999f);

          // The height should be close to the final height.
          FVector llh = pGlobeAnchor->GetLongitudeLatitudeHeight();
          TestTrue(
              "Height is close to final height",
              FMath::IsNearlyEqual(llh.Z, 100.0, 10.0));

          pPawn->Destroy();
        });
      });
}

#endif
