// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "CesiumGeoreference.h"
#include "CesiumOriginShiftComponent.h"
#include "CesiumSubLevelComponent.h"
#include "CesiumTestHelpers.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GlobeAwareDefaultPawn.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"

BEGIN_DEFINE_SPEC(
    FSubLevelsSpec,
    "Cesium.Unit.SubLevels",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)

TObjectPtr<UWorld> pWorld;
TObjectPtr<ALevelInstance> pSubLevel1;
TObjectPtr<UCesiumSubLevelComponent> pLevelComponent1;
TObjectPtr<ALevelInstance> pSubLevel2;
TObjectPtr<UCesiumSubLevelComponent> pLevelComponent2;
TObjectPtr<ACesiumGeoreference> pGeoreference;
TObjectPtr<AGlobeAwareDefaultPawn> pPawn;
FDelegateHandle subscriptionPostPIEStarted;

END_DEFINE_SPEC(FSubLevelsSpec)

using namespace CesiumTestHelpers;

void FSubLevelsSpec::Define() {
  BeforeEach([this]() {
    if (IsValid(pWorld)) {
      // Only run the below once in order to save time loading/unloading
      // levels for every little test.
      return;
    }

    pWorld = FAutomationEditorCommonUtils::CreateNewMap();

    pSubLevel1 = pWorld->SpawnActor<ALevelInstance>();
    trackForPlay(pSubLevel1);
    pSubLevel1->SetWorldAsset(TSoftObjectPtr<UWorld>(
        FSoftObjectPath("/CesiumForUnreal/Tests/Maps/SingleCube.SingleCube")));
    pSubLevel1->SetIsTemporarilyHiddenInEditor(true);
    pLevelComponent1 =
        Cast<UCesiumSubLevelComponent>(pSubLevel1->AddComponentByClass(
            UCesiumSubLevelComponent::StaticClass(),
            false,
            FTransform::Identity,
            false));
    trackForPlay(pLevelComponent1);
    pLevelComponent1->SetOriginLongitudeLatitudeHeight(
        FVector(10.0, 20.0, 1000.0));
    pSubLevel1->AddInstanceComponent(pLevelComponent1);

    pSubLevel2 = pWorld->SpawnActor<ALevelInstance>();
    trackForPlay(pSubLevel2);
    pSubLevel2->SetWorldAsset(TSoftObjectPtr<UWorld>(FSoftObjectPath(
        "/CesiumForUnreal/Tests/Maps/ConeAndCylinder.ConeAndCylinder")));
    pSubLevel2->SetIsTemporarilyHiddenInEditor(true);
    pLevelComponent2 =
        Cast<UCesiumSubLevelComponent>(pSubLevel2->AddComponentByClass(
            UCesiumSubLevelComponent::StaticClass(),
            false,
            FTransform::Identity,
            false));
    trackForPlay(pLevelComponent2);
    pLevelComponent2->SetOriginLongitudeLatitudeHeight(
        FVector(-25.0, 15.0, -5000.0));
    pSubLevel2->AddInstanceComponent(pLevelComponent2);

    pSubLevel1->LoadLevelInstance();
    pSubLevel2->LoadLevelInstance();

    pGeoreference = nullptr;
    for (TActorIterator<ACesiumGeoreference> it(pWorld); it; ++it) {
      pGeoreference = *it;
    }

    trackForPlay(pGeoreference);

    pPawn = pWorld->SpawnActor<AGlobeAwareDefaultPawn>();
    pPawn->AddComponentByClass(
        UCesiumOriginShiftComponent::StaticClass(),
        false,
        FTransform::Identity,
        false);
    trackForPlay(pPawn);
    pPawn->AutoPossessPlayer = EAutoReceiveInput::Player0;
  });

  AfterEach([this]() {
    pSubLevel1->SetIsTemporarilyHiddenInEditor(true);
    pSubLevel2->SetIsTemporarilyHiddenInEditor(true);
  });

  It("initially hides sub-levels in the Editor", [this]() {
    TestTrue("pGeoreference is valid", IsValid(pGeoreference));
    TestTrue("pSubLevel1 is valid", IsValid(pSubLevel1));
    TestTrue("pSubLevel2 is valid", IsValid(pSubLevel2));
    TestTrue(
        "pSubLevel1 is hidden",
        pSubLevel1->IsTemporarilyHiddenInEditor(true));
    TestTrue(
        "pSubLevel2 is hidden",
        pSubLevel1->IsTemporarilyHiddenInEditor(true));
  });

  Describe(
      "copies CesiumGeoreference origin changes to the active sub-level in the Editor",
      [this]() {
        BeforeEach(EAsyncExecution::TaskGraphMainThread, [this]() {
          pSubLevel1->SetIsTemporarilyHiddenInEditor(false);
        });
        It("", EAsyncExecution::TaskGraphMainThread, [this]() {
          TestFalse(
              "pSubLevel1 is hidden",
              pSubLevel1->IsTemporarilyHiddenInEditor(true));

          pGeoreference->SetOriginLongitudeLatitudeHeight(
              FVector(1.0, 2.0, 3.0));
          TestEqual("Longitude", pLevelComponent1->GetOriginLongitude(), 1.0);
          TestEqual("Latitude", pLevelComponent1->GetOriginLatitude(), 2.0);
          TestEqual("Height", pLevelComponent1->GetOriginHeight(), 3.0);
        });
      });

  Describe(
      "copies active sub-level origin changes to the CesiumGeoreference in the Editor",
      [this]() {
        BeforeEach(EAsyncExecution::TaskGraphMainThread, [this]() {
          pSubLevel1->SetIsTemporarilyHiddenInEditor(false);
        });
        It("", EAsyncExecution::TaskGraphMainThread, [this]() {
          TestFalse(
              "pSubLevel1 is hidden",
              pSubLevel1->IsTemporarilyHiddenInEditor(true));

          pLevelComponent1->SetOriginLongitudeLatitudeHeight(
              FVector(4.0, 5.0, 6.0));
          TestEqual("Longitude", pGeoreference->GetOriginLongitude(), 4.0);
          TestEqual("Latitude", pGeoreference->GetOriginLatitude(), 5.0);
          TestEqual("Height", pGeoreference->GetOriginHeight(), 6.0);
        });
      });

  Describe(
      "does not copy inactive sub-level origin changes to the CesiumGeoreference in the Editor",
      [this]() {
        BeforeEach(EAsyncExecution::TaskGraphMainThread, [this]() {
          // Set the sub-level active, wait a frame so it becomes so.
          pSubLevel1->SetIsTemporarilyHiddenInEditor(false);
        });
        BeforeEach(EAsyncExecution::TaskGraphMainThread, [this]() {
          // Set the sub-level inactive, wait a frame so it becomes so.
          pSubLevel1->SetIsTemporarilyHiddenInEditor(true);
        });
        It("", EAsyncExecution::TaskGraphMainThread, [this]() {
          // Verify that the previously-active sub-level isn't affected by
          // georeference origin changes.
          double expectedLongitude = pGeoreference->GetOriginLongitude();
          double expectedLatitude = pGeoreference->GetOriginLatitude();
          double expectedHeight = pGeoreference->GetOriginHeight();

          TestNotEqual("Longitude", expectedLongitude, 7.0);
          TestNotEqual("Latitude", expectedLatitude, 8.0);
          TestNotEqual("Height", expectedHeight, 9.0);

          pLevelComponent1->SetOriginLongitudeLatitudeHeight(
              FVector(7.0, 8.0, 9.0));
          TestEqual(
              "Longitude",
              pGeoreference->GetOriginLongitude(),
              expectedLongitude);
          TestEqual(
              "Latitude",
              pGeoreference->GetOriginLatitude(),
              expectedLatitude);
          TestEqual("Height", pGeoreference->GetOriginHeight(), expectedHeight);
        });
      });

  Describe(
      "ensures only one sub-level instance is visible in the editor at a time",
      [this]() {
        BeforeEach(EAsyncExecution::TaskGraphMainThread, [this]() {
          pSubLevel1->SetIsTemporarilyHiddenInEditor(false);
        });
        BeforeEach(EAsyncExecution::TaskGraphMainThread, [this]() {
          TestFalse(
              "pSubLevel1 is hidden",
              pSubLevel1->IsTemporarilyHiddenInEditor(true));
          TestTrue(
              "pSubLevel2 is hidden",
              pSubLevel2->IsTemporarilyHiddenInEditor(true));

          pSubLevel2->SetIsTemporarilyHiddenInEditor(false);
        });
        It("", EAsyncExecution::TaskGraphMainThread, [this]() {
          TestTrue(
              "pSubLevel1 is hidden",
              pSubLevel1->IsTemporarilyHiddenInEditor(true));
          TestFalse(
              "pSubLevel2 is hidden",
              pSubLevel2->IsTemporarilyHiddenInEditor(true));
        });
      });

  Describe("activates the closest sub-level that is in range", [this]() {
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
    });
    LatentBeforeEach(
        EAsyncExecution::TaskGraphMainThread,
        [this](const FDoneDelegate& done) {
          waitFor(done, GEditor->PlayWorld, 5.0f, [this]() {
            return !findInPlay(pSubLevel1)->IsLoaded() &&
                   !findInPlay(pSubLevel2)->IsLoaded();
          });
        });
    BeforeEach(EAsyncExecution::TaskGraphMainThread, [this]() {
      TestFalse("pSubLevel1 is loaded", findInPlay(pSubLevel1)->IsLoaded());
      TestFalse("pSubLevel2 is loaded", findInPlay(pSubLevel2)->IsLoaded());

      // Move the player within range of the first sub-level
      UCesiumSubLevelComponent* pPlayLevelComponent1 =
          findInPlay(pLevelComponent1);
      FVector origin1 =
          findInPlay(pGeoreference)
              ->TransformLongitudeLatitudeHeightPositionToUnreal(FVector(
                  pPlayLevelComponent1->GetOriginLongitude(),
                  pPlayLevelComponent1->GetOriginLatitude(),
                  pPlayLevelComponent1->GetOriginHeight()));
      findInPlay(pPawn)->SetActorLocation(origin1);
    });
    LatentBeforeEach(
        EAsyncExecution::TaskGraphMainThread,
        [this](const FDoneDelegate& done) {
          waitFor(done, GEditor->PlayWorld, 5.0f, [this]() {
            return findInPlay(pSubLevel1)->IsLoaded();
          });
        });
    It("", EAsyncExecution::TaskGraphMainThread, [this]() {
      TestTrue("pSubLevel1 is loaded", findInPlay(pSubLevel1)->IsLoaded());
      TestFalse("pSubLevel2 is loaded", findInPlay(pSubLevel2)->IsLoaded());
    });
    AfterEach(EAsyncExecution::TaskGraphMainThread, [this]() {
      GEditor->RequestEndPlayMap();
    });
  });

  Describe("handles a rapid load / unload / reload cycle", [this]() {
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
    });
    LatentBeforeEach(
        EAsyncExecution::TaskGraphMainThread,
        [this](const FDoneDelegate& done) {
          waitFor(done, GEditor->PlayWorld, 5.0f, [this]() {
            return !findInPlay(pSubLevel1)->IsLoaded() &&
                   !findInPlay(pSubLevel2)->IsLoaded();
          });
        });
    BeforeEach(EAsyncExecution::TaskGraphMainThread, [this]() {
      TestFalse("pSubLevel1 is loaded", findInPlay(pSubLevel1)->IsLoaded());
      TestFalse("pSubLevel2 is loaded", findInPlay(pSubLevel2)->IsLoaded());

      // Move the player within range of the first sub-level
      UCesiumSubLevelComponent* pPlayLevelComponent1 =
          findInPlay(pLevelComponent1);
      FVector origin1 =
          findInPlay(pGeoreference)
              ->TransformLongitudeLatitudeHeightPositionToUnreal(FVector(
                  pPlayLevelComponent1->GetOriginLongitude(),
                  pPlayLevelComponent1->GetOriginLatitude(),
                  pPlayLevelComponent1->GetOriginHeight()));
      findInPlay(pPawn)->SetActorLocation(origin1);
    });
    LatentBeforeEach(
        EAsyncExecution::TaskGraphMainThread,
        [this](const FDoneDelegate& done) {
          // Wait for the level to load.
          waitFor(done, GEditor->PlayWorld, 5.0f, [this]() {
            return findInPlay(pSubLevel1)->IsLoaded();
          });
        });
    BeforeEach(EAsyncExecution::TaskGraphMainThread, [this]() {
      // Move the player outside the sub-level, triggering an unload.
      UCesiumSubLevelComponent* pPlayLevelComponent1 =
          findInPlay(pLevelComponent1);
      FVector origin1 =
          findInPlay(pGeoreference)
              ->TransformLongitudeLatitudeHeightPositionToUnreal(FVector(
                  pPlayLevelComponent1->GetOriginLongitude(),
                  pPlayLevelComponent1->GetOriginLatitude(),
                  pPlayLevelComponent1->GetOriginHeight() + 100000.0));
      findInPlay(pPawn)->SetActorLocation(origin1);
    });
    BeforeEach(EAsyncExecution::TaskGraphMainThread, [this]() {
      // Without waiting for the level to finish unloading, move the player back
      // inside it.
      UCesiumSubLevelComponent* pPlayLevelComponent1 =
          findInPlay(pLevelComponent1);
      FVector origin1 =
          findInPlay(pGeoreference)
              ->TransformLongitudeLatitudeHeightPositionToUnreal(FVector(
                  pPlayLevelComponent1->GetOriginLongitude(),
                  pPlayLevelComponent1->GetOriginLatitude(),
                  pPlayLevelComponent1->GetOriginHeight()));
      findInPlay(pPawn)->SetActorLocation(origin1);
    });
    LatentBeforeEach(
        EAsyncExecution::TaskGraphMainThread,
        [this](const FDoneDelegate& done) {
          // Wait for the level to load.
          waitFor(done, GEditor->PlayWorld, 5.0f, [this]() {
            return findInPlay(pSubLevel1)->IsLoaded();
          });
        });
    It("", EAsyncExecution::TaskGraphMainThread, [this]() {
      TestTrue("pSubLevel1 is loaded", findInPlay(pSubLevel1)->IsLoaded());
      TestFalse("pSubLevel2 is loaded", findInPlay(pSubLevel2)->IsLoaded());
    });
    AfterEach(EAsyncExecution::TaskGraphMainThread, [this]() {
      GEditor->RequestEndPlayMap();
    });
  });
}

#endif // #if WITH_EDITOR
