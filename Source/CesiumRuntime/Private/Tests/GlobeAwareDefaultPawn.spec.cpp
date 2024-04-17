// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "CesiumFlyToComponent.h"
#include "CesiumGeoreference.h"
#include "CesiumGlobeAnchorComponent.h"
#include "CesiumTestHelpers.h"
#include "CesiumWgs84Ellipsoid.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GlobeAwareDefaultPawn.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"

BEGIN_DEFINE_SPEC(
    FGlobeAwareDefaultPawnSpec,
    "Cesium.Unit.GlobeAwareDefaultPawn",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)

FDelegateHandle subscriptionPostPIEStarted;

END_DEFINE_SPEC(FGlobeAwareDefaultPawnSpec)

void FGlobeAwareDefaultPawnSpec::Define() {
  const FVector philadelphiaEcef =
      FVector(1253264.69280105, -4732469.91065521, 4075112.40412297);

  // The antipodal position from the philadelphia coordinates above
  const FVector philadelphiaAntipodeEcef =
      FVector(-1253369.920224856, 4732412.7444064, -4075146.2160252854);

  const FVector tokyoEcef =
      FVector(-3960158.65587452, 3352568.87555906, 3697235.23506459);

  Describe(
      "should not spike altitude when very close to final destination",
      [this]() {
        LatentBeforeEach(
            EAsyncExecution::TaskGraphMainThread,
            [this](const FDoneDelegate& done) {
              UWorld* pWorld = FAutomationEditorCommonUtils::CreateNewMap();

              AGlobeAwareDefaultPawn* pPawn =
                  pWorld->SpawnActor<AGlobeAwareDefaultPawn>();
              UCesiumFlyToComponent* pFlyTo =
                  Cast<UCesiumFlyToComponent>(pPawn->AddComponentByClass(
                      UCesiumFlyToComponent::StaticClass(),
                      false,
                      FTransform::Identity,
                      false));
              pFlyTo->RotationToUse =
                  ECesiumFlyToRotation::ControlRotationInEastSouthUp;

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
          UCesiumFlyToComponent* pFlyTo =
              pPawn->FindComponentByClass<UCesiumFlyToComponent>();
          TestNotNull("pFlyTo", pFlyTo);
          pFlyTo->Duration = 5.0f;

          UCesiumGlobeAnchorComponent* pGlobeAnchor =
              pPawn->FindComponentByClass<UCesiumGlobeAnchorComponent>();
          TestNotNull("pGlobeAnchor", pGlobeAnchor);

          // Start flying somewhere else
          pFlyTo->FlyToLocationLongitudeLatitudeHeight(
              FVector(25.0, 10.0, 100.0),
              0.0,
              0.0,
              false);

          // Tick almost to the end
          Cast<UActorComponent>(pFlyTo)->TickComponent(
              4.9999f,
              ELevelTick::LEVELTICK_All,
              nullptr);

          // The height should be close to the final height.
          FVector llh = pGlobeAnchor->GetLongitudeLatitudeHeight();
          TestTrue(
              "Height is close to final height",
              FMath::IsNearlyEqual(llh.Z, 100.0, 10.0));

          pPawn->Destroy();
        });
      });

  Describe(
      "should interpolate between positions and rotations correctly",
      [this, tokyoEcef, philadelphiaEcef, philadelphiaAntipodeEcef]() {
        LatentBeforeEach(
            EAsyncExecution::TaskGraphMainThread,
            [this](const FDoneDelegate& done) {
              UWorld* pWorld = FAutomationEditorCommonUtils::CreateNewMap();

              AGlobeAwareDefaultPawn* pPawn =
                  pWorld->SpawnActor<AGlobeAwareDefaultPawn>();
              UCesiumFlyToComponent* pFlyTo =
                  Cast<UCesiumFlyToComponent>(pPawn->AddComponentByClass(
                      UCesiumFlyToComponent::StaticClass(),
                      false,
                      FTransform::Identity,
                      false));
              pFlyTo->RotationToUse =
                  ECesiumFlyToRotation::ControlRotationInEastSouthUp;

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
        It("should match the beginning and ending points of the fly-to",
           [this]() {
             UWorld* pWorld = GEditor->PlayWorld;

             TActorIterator<AGlobeAwareDefaultPawn> it(pWorld);
             AGlobeAwareDefaultPawn* pPawn = *it;
             UCesiumFlyToComponent* pFlyTo =
                 pPawn->FindComponentByClass<UCesiumFlyToComponent>();
             TestNotNull("pFlyTo", pFlyTo);
             pFlyTo->Duration = 5.0f;

             UCesiumGlobeAnchorComponent* pGlobeAnchor =
                 pPawn->FindComponentByClass<UCesiumGlobeAnchorComponent>();
             TestNotNull("pGlobeAnchor", pGlobeAnchor);

             pGlobeAnchor->MoveToLongitudeLatitudeHeight(
                 FVector(25.0, 10.0, 100.0));

             // Start flying somewhere else
             pFlyTo->FlyToLocationLongitudeLatitudeHeight(
                 FVector(25.0, 25.0, 100.0),
                 0.0,
                 0.0,
                 false);

             TestEqual(
                 "Location is the same as the start point",
                 pGlobeAnchor->GetLongitudeLatitudeHeight(),
                 FVector(25.0, 10.0, 100.0));

             // Tick to the end
             Cast<UActorComponent>(pFlyTo)->TickComponent(
                 5.0f,
                 ELevelTick::LEVELTICK_All,
                 nullptr);

             TestEqual(
                 "Location is the same as the end point",
                 pGlobeAnchor->GetLongitudeLatitudeHeight(),
                 FVector(25.0, 25.0, 100.0));

             pPawn->Destroy();
           });
        It("should correctly compute the midpoint of the flight",
           [this, tokyoEcef, philadelphiaEcef]() {
             UWorld* pWorld = GEditor->PlayWorld;

             TActorIterator<AGlobeAwareDefaultPawn> it(pWorld);
             AGlobeAwareDefaultPawn* pPawn = *it;
             UCesiumFlyToComponent* pFlyTo =
                 pPawn->FindComponentByClass<UCesiumFlyToComponent>();
             TestNotNull("pFlyTo", pFlyTo);
             pFlyTo->Duration = 5.0f;
             pFlyTo->HeightPercentageCurve = nullptr;
             pFlyTo->MaximumHeightByDistanceCurve = nullptr;
             pFlyTo->ProgressCurve = nullptr;

             UCesiumGlobeAnchorComponent* pGlobeAnchor =
                 pPawn->FindComponentByClass<UCesiumGlobeAnchorComponent>();
             TestNotNull("pGlobeAnchor", pGlobeAnchor);

             pGlobeAnchor->MoveToEarthCenteredEarthFixedPosition(
                 philadelphiaEcef);
             pFlyTo->FlyToLocationEarthCenteredEarthFixed(tokyoEcef, 0, 0, 0);

             // Tick half way through
             Cast<UActorComponent>(pFlyTo)->TickComponent(
                 2.5f,
                 ELevelTick::LEVELTICK_All,
                 nullptr);

             FVector expectedResult = FVector(
                 -2062499.3622640674,
                 -1052346.4221710551,
                 5923430.4378960524);

             // calculate relative epsilon
             const float epsilon = FMath::Max3(
                                       expectedResult.X,
                                       expectedResult.Y,
                                       expectedResult.Z) *
                                   1e-6;

             TestEqual(
                 "Midpoint location is correct",
                 pGlobeAnchor->GetEarthCenteredEarthFixedPosition(),
                 expectedResult,
                 epsilon);

             pPawn->Destroy();
           });
        It("should match the start and end rotations",
           [this, tokyoEcef, philadelphiaEcef]() {
             UWorld* pWorld = GEditor->PlayWorld;

             TActorIterator<AGlobeAwareDefaultPawn> it(pWorld);
             AGlobeAwareDefaultPawn* pPawn = *it;
             UCesiumFlyToComponent* pFlyTo =
                 pPawn->FindComponentByClass<UCesiumFlyToComponent>();
             TestNotNull("pFlyTo", pFlyTo);
             pFlyTo->Duration = 5.0f;
             pFlyTo->HeightPercentageCurve = nullptr;
             pFlyTo->MaximumHeightByDistanceCurve = nullptr;
             pFlyTo->ProgressCurve = nullptr;

             UCesiumGlobeAnchorComponent* pGlobeAnchor =
                 pPawn->FindComponentByClass<UCesiumGlobeAnchorComponent>();
             TestNotNull("pGlobeAnchor", pGlobeAnchor);

             FQuat sourceRotation = FRotator(0, 0, 0).Quaternion();
             FQuat targetRotation = FRotator(45, 180, 0).Quaternion();
             FQuat midpointRotation =
                 FQuat::Slerp(sourceRotation, targetRotation, 0.5);

             pGlobeAnchor->MoveToEarthCenteredEarthFixedPosition(
                 philadelphiaEcef);
             pGlobeAnchor->SetEastSouthUpRotation(sourceRotation);
             pFlyTo->FlyToLocationEarthCenteredEarthFixed(
                 tokyoEcef,
                 180,
                 45,
                 false);

             TestTrue(
                 "Start rotation is correct",
                 pPawn->Controller->GetControlRotation().Quaternion().Equals(
                     sourceRotation,
                     CesiumUtility::Math::Epsilon4));

             // Tick half way through
             Cast<UActorComponent>(pFlyTo)->TickComponent(
                 2.5f,
                 ELevelTick::LEVELTICK_All,
                 nullptr);

             TestTrue(
                 "Midpoint rotation is correct",
                 pPawn->Controller->GetControlRotation().Quaternion().Equals(
                     midpointRotation,
                     CesiumUtility::Math::Epsilon4));

             FVector currentEastSouthRotationEuler =
                 pGlobeAnchor->GetEastSouthUpRotation().Euler();
             FVector midpointEuler = midpointRotation.Euler();

             // Tick to the end
             Cast<UActorComponent>(pFlyTo)->TickComponent(
                 2.5f,
                 ELevelTick::LEVELTICK_All,
                 nullptr);

             TestTrue(
                 "End location is correct",
                 pPawn->Controller->GetControlRotation().Quaternion().Equals(
                     targetRotation,
                     CesiumUtility::Math::Epsilon4));

             FVector endEastSouthRotationEuler =
                 pGlobeAnchor->GetEastSouthUpRotation().Euler();
             FVector endpointEuler = targetRotation.Euler();

             pPawn->Destroy();
           });
        It("shouldn't fly through the earth",
           [this, philadelphiaEcef, philadelphiaAntipodeEcef]() {
             UWorld* pWorld = GEditor->PlayWorld;

             TActorIterator<AGlobeAwareDefaultPawn> it(pWorld);
             AGlobeAwareDefaultPawn* pPawn = *it;
             UCesiumFlyToComponent* pFlyTo =
                 pPawn->FindComponentByClass<UCesiumFlyToComponent>();
             TestNotNull("pFlyTo", pFlyTo);

             pFlyTo->Duration = 5.0f;
             pFlyTo->HeightPercentageCurve = nullptr;
             pFlyTo->MaximumHeightByDistanceCurve = nullptr;
             pFlyTo->ProgressCurve = nullptr;

             UCesiumGlobeAnchorComponent* pGlobeAnchor =
                 pPawn->FindComponentByClass<UCesiumGlobeAnchorComponent>();
             TestNotNull("pGlobeAnchor", pGlobeAnchor);

             pGlobeAnchor->MoveToEarthCenteredEarthFixedPosition(
                 philadelphiaEcef);
             pFlyTo->FlyToLocationEarthCenteredEarthFixed(
                 philadelphiaAntipodeEcef,
                 0,
                 0,
                 false);

             const int steps = 100;
             const double timeStep = 5.0 / (double)steps;

             double time = 0;
             for (int i = 0; i <= steps; i++) {
               Cast<UActorComponent>(pFlyTo)->TickComponent(
                   (float)timeStep,
                   ELevelTick::LEVELTICK_All,
                   nullptr);

               const FVector cartographic = UCesiumWgs84Ellipsoid::
                   EarthCenteredEarthFixedToLongitudeLatitudeHeight(
                       pGlobeAnchor->GetEarthCenteredEarthFixedPosition());

               TestTrue("height above zero", cartographic.Z > 0);

               time += timeStep;
             }
           });
      });
}

#endif
