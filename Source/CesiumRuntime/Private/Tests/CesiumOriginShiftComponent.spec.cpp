// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "CesiumOriginShiftComponent.h"
#include "CesiumGeoreference.h"
#include "CesiumGlobeAnchorComponent.h"
#include "CesiumSubLevelComponent.h"
#include "CesiumTestHelpers.h"
#include "Editor.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"

BEGIN_DEFINE_SPEC(
    FCesiumOriginShiftComponentSpec,
    "Cesium.Unit.OriginShiftComponent",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)

TObjectPtr<UWorld> pWorld;
TObjectPtr<ACesiumGeoreference> pGeoreference;
TObjectPtr<AStaticMeshActor> pOriginShiftActor;
TObjectPtr<UCesiumOriginShiftComponent> pOriginShiftComponent;
FDelegateHandle subscriptionPostPIEStarted;

END_DEFINE_SPEC(FCesiumOriginShiftComponentSpec)

using namespace CesiumTestHelpers;

void FCesiumOriginShiftComponentSpec::Define() {
  BeforeEach([this]() {
    if (IsValid(pWorld)) {
      // Only run the below once in order to save time loading/unloading
      // levels for every little test.
      return;
    }

    pWorld = FAutomationEditorCommonUtils::CreateNewMap();

    pOriginShiftActor = pWorld->SpawnActor<AStaticMeshActor>();
    pOriginShiftActor->SetMobility(EComponentMobility::Movable);
    trackForPlay(pOriginShiftActor);

    pOriginShiftComponent = Cast<UCesiumOriginShiftComponent>(
        pOriginShiftActor->AddComponentByClass(
            UCesiumOriginShiftComponent::StaticClass(),
            false,
            FTransform::Identity,
            false));
    trackForPlay(pOriginShiftComponent);

    pGeoreference = nullptr;
    for (TActorIterator<ACesiumGeoreference> it(pWorld); it; ++it) {
      pGeoreference = *it;
    }

    trackForPlay(pGeoreference);
  });

  AfterEach([this]() {});

  It("automatically adds a globe anchor to go with the origin shift", [this]() {
    UCesiumGlobeAnchorComponent* pGlobeAnchor =
        pOriginShiftActor->FindComponentByClass<UCesiumGlobeAnchorComponent>();
    TestNotNull("pGlobeAnchor", pGlobeAnchor);
  });

  Describe(
      "does not shift origin when in between sub-levels when mode is SwitchSubLevelsOnly",
      [this]() {
        LatentBeforeEach(
            EAsyncExecution::TaskGraphMainThread,
            [this](const FDoneDelegate& done) {
              subscriptionPostPIEStarted =
                  FEditorDelegates::PostPIEStarted.AddLambda(
                      [done](bool isSimulating) { done.Execute(); });
              FRequestPlaySessionParams params{};
              GEditor->RequestPlaySession(params);
            });
        BeforeEach(EAsyncExecution::TaskGraphMainThread, [this]() {
          FEditorDelegates::PostPIEStarted.Remove(subscriptionPostPIEStarted);

          findInPlay(pOriginShiftActor)
              ->SetActorLocation(FVector(10000.0, 20000.0, 300.0));
        });
        It("", [this]() {
          TestEqual(
              "location",
              findInPlay(pOriginShiftActor)->GetActorLocation(),
              FVector(10000.0, 20000.0, 300.0));
        });
        AfterEach(EAsyncExecution::TaskGraphMainThread, [this]() {
          GEditor->RequestEndPlayMap();
        });
      });

  Describe(
      "shifts origin by changing georeference when mode is ChangeCesiumGeoreference",
      [this]() {
        LatentBeforeEach(
            EAsyncExecution::TaskGraphMainThread,
            [this](const FDoneDelegate& done) {
              subscriptionPostPIEStarted =
                  FEditorDelegates::PostPIEStarted.AddLambda(
                      [done](bool isSimulating) { done.Execute(); });
              FRequestPlaySessionParams params{};
              GEditor->RequestPlaySession(params);
            });
        BeforeEach(EAsyncExecution::TaskGraphMainThread, [this]() {
          FEditorDelegates::PostPIEStarted.Remove(subscriptionPostPIEStarted);

          // Start with the Actor at the origin at LLH 0,0,0.
          UCesiumGlobeAnchorComponent* pGlobeAnchor =
              findInPlay(pOriginShiftActor)
                  ->FindComponentByClass<UCesiumGlobeAnchorComponent>();
          pGlobeAnchor->MoveToLongitudeLatitudeHeight(FVector(0.0, 0.0, 0.0));
          findInPlay(pGeoreference)
              ->SetOriginLongitudeLatitudeHeight(FVector(0.0, 0.0, 0.0));
          pGlobeAnchor->SnapToEastSouthUp();

          // Activate georeference origin shifting
          findInPlay(pOriginShiftComponent)
              ->SetMode(ECesiumOriginShiftMode::ChangeCesiumGeoreference);

          // Move it to 90 degrees longitude.
          FVector location = FVector(
              CesiumGeospatial::Ellipsoid::WGS84.getMaximumRadius() * 100.0,
              0.0,
              -CesiumGeospatial::Ellipsoid::WGS84.getMaximumRadius() * 100.0);
          findInPlay(pOriginShiftActor)->SetActorLocation(location);
          TestEqual("Longitude", pGlobeAnchor->GetLongitude(), 90.0);
          TestEqual("Latitude", pGlobeAnchor->GetLatitude(), 0.0);
          TestEqual("Height", pGlobeAnchor->GetHeight(), 0.0);
          TestTrue(
              "Rotation",
              pGlobeAnchor->GetEastSouthUpRotation().Equals(FQuat::Identity));
        });
        It("", [this]() {
          TestEqual(
              "location",
              findInPlay(pOriginShiftActor)->GetActorLocation(),
              FVector::Zero());

          UCesiumGlobeAnchorComponent* pGlobeAnchor =
              findInPlay(pOriginShiftActor)
                  ->FindComponentByClass<UCesiumGlobeAnchorComponent>();
          TestEqual("Longitude", pGlobeAnchor->GetLongitude(), 90.0);
          TestEqual("Latitude", pGlobeAnchor->GetLatitude(), 0.0);
          TestEqual("Height", pGlobeAnchor->GetHeight(), 0.0);

          // The Actor should still be aligned with the new East-South-Up
          // because moving it will rotate it for globe curvature.
          TestTrue(
              "Rotation",
              pGlobeAnchor->GetEastSouthUpRotation().Equals(FQuat::Identity));
        });
        AfterEach(EAsyncExecution::TaskGraphMainThread, [this]() {
          GEditor->RequestEndPlayMap();
        });
      });
}

#endif // #if WITH_EDITOR
