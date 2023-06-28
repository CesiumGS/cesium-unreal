#if WITH_EDITOR

#include "CesiumAsync/AsyncSystem.h"
#include "CesiumGeoreference.h"
#include "CesiumSubLevelComponent.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GlobeAwareDefaultPawn.h"
#include "Kismet/GameplayStatics.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "TimerManager.h"
#include <functional>

BEGIN_DEFINE_SPEC(
    FSubLevelsSpec,
    "Cesium.SubLevels",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)

CesiumAsync::AsyncSystem asyncSystem{nullptr};
TObjectPtr<UWorld> pWorld;
TObjectPtr<ALevelInstance> pSubLevel1;
TObjectPtr<UCesiumSubLevelComponent> pLevelComponent1;
TObjectPtr<ALevelInstance> pSubLevel2;
TObjectPtr<UCesiumSubLevelComponent> pLevelComponent2;
TObjectPtr<ACesiumGeoreference> pGeoreference;
TObjectPtr<AGlobeAwareDefaultPawn> pPawn;
FDelegateHandle subscriptionPostPIEStarted;

END_DEFINE_SPEC(FSubLevelsSpec)

namespace {

// template <typename T>
// void waitForAsync(
//     const FDoneDelegate& done,
//     UWorld* pWorld,
//     CesiumAsync::AsyncSystem& asyncSystem,
//     CesiumAsync::Future<T>&& future) {
//   struct WaitDetails {
//     CesiumAsync::AsyncSystem asyncSystem;
//     bool terminate;
//     const FDoneDelegate done;
//     std::function<void()> tick;
//   };
//
//   auto pDetails = std::make_shared<WaitDetails>(
//       WaitDetails{asyncSystem, false, done, std::function<void()>()});
//
//   pDetails->tick = [pDetails, pWorld]() {
//     if (pDetails->terminate) {
//       pDetails->done.Execute();
//     } else {
//       pWorld->GetTimerManager().SetTimerForNextTick([pDetails]() {
//         pDetails->asyncSystem.dispatchMainThreadTasks();
//         pDetails->tick();
//       });
//     }
//   };
//
//   std::move(future).thenInMainThread(
//       [pDetails]() { pDetails->terminate = true; });
//
//   pDetails->tick();
// }
//
// template <typename T>
// auto waitForNextFrame(
//     UWorld* pWorld,
//     const CesiumAsync::AsyncSystem& asyncSystem) {
//   return [asyncSystem, pWorld](T&& value) {
//     CesiumAsync::Promise<T> promise = asyncSystem.createPromise<T>();
//     pWorld->GetTimerManager().SetTimerForNextTick(
//         [promise, value]() { promise.resolve(std::move(value)); });
//     return promise.getFuture();
//   };
// }
//
// template <>
// auto waitForNextFrame<void>(
//     UWorld* pWorld,
//     const CesiumAsync::AsyncSystem& asyncSystem) {
//   return [asyncSystem, pWorld]() {
//     CesiumAsync::Promise<void> promise = asyncSystem.createPromise<void>();
//     pWorld->GetTimerManager().SetTimerForNextTick(
//         [promise]() { promise.resolve(); });
//     return promise.getFuture();
//   };
// }

template <typename T>
void WaitForImpl(
    const FDoneDelegate& done,
    UWorld* pWorld,
    T&& condition,
    FTimerHandle& timerHandle) {
  if (condition()) {
    pWorld->GetTimerManager().ClearTimer(timerHandle);
    done.Execute();
  } else if (pWorld->GetTimerManager().GetTimerRemaining(timerHandle) <= 0.0f) {
    // Timeout
    UE_LOG(
        LogCesium,
        Error,
        TEXT("Timed out waiting for a condition to become true."));
    pWorld->GetTimerManager().ClearTimer(timerHandle);
    done.Execute();
  } else {
    pWorld->GetTimerManager().SetTimerForNextTick(
        [done, pWorld, condition, timerHandle]() mutable {
          WaitForImpl<T>(done, pWorld, std::move(condition), timerHandle);
        });
  }
}

template <typename T>
void WaitFor(
    const FDoneDelegate& done,
    UWorld* pWorld,
    float timeoutSeconds,
    T&& condition) {
  FTimerHandle timerHandle;
  pWorld->GetTimerManager().SetTimer(timerHandle, timeoutSeconds, false);
  WaitForImpl<T>(done, pWorld, std::forward<T>(condition), timerHandle);
}

template <typename T> T* GetActorWithTag(UWorld* pWorld, const FName& tag) {
  TArray<AActor*> temp;
  UGameplayStatics::GetAllActorsWithTag(pWorld, tag, temp);
  if (temp.Num() < 1)
    return nullptr;

  return Cast<T>(temp[0]);
}

template <typename T> T* GetComponentWithTag(AActor* pOwner, const FName& tag) {
  TArray<UActorComponent*> components =
      pOwner->GetComponentsByTag(T::StaticClass(), tag);
  if (components.Num() < 1)
    return nullptr;

  return Cast<T>(components[0]);
}

FName GetTag(AActor* pActor) {
  return FName(FString::Printf(TEXT("%lld"), pActor));
}

FName GetTag(UActorComponent* pComponent) {
  return FName(FString::Printf(TEXT("%lld"), pComponent));
}

void Track(AActor* pActor) { pActor->Tags.Add(GetTag(pActor)); }

void Track(UActorComponent* pComponent) {
  Track(pComponent->GetOwner());
  pComponent->ComponentTags.Add(GetTag(pComponent));
}

template <typename T> T* FindInPlay(T* pEditorObject) {
  UWorld* pWorld = GEditor->PlayWorld;
  if constexpr (std::is_base_of_v<AActor, T>) {
    return GetActorWithTag<T>(pWorld, GetTag(pEditorObject));
  } else if constexpr (std::is_base_of_v<UActorComponent, T>) {
    AActor* pEditorOwner = pEditorObject->GetOwner();
    if (!pEditorOwner)
      return nullptr;
    AActor* pPlayOwner = FindInPlay(pEditorOwner);
    if (!pPlayOwner)
      return nullptr;
    return GetComponentWithTag<T>(pPlayOwner, GetTag(pEditorObject));
  }
}

template <typename T> T* FindInPlay(TObjectPtr<T> pEditorObject) {
  return FindInPlay<T>(pEditorObject.Get());
}

} // namespace

void FSubLevelsSpec::Define() {
  BeforeEach([this]() {
    if (pWorld != nullptr) {
      // Only run the below once in order to save time loading/unloading
      // levels for every little test.
      return;
    }

    pWorld = FAutomationEditorCommonUtils::CreateNewMap();

    pSubLevel1 = pWorld->SpawnActor<ALevelInstance>();
    Track(pSubLevel1);
    pSubLevel1->SetWorldAsset(TSoftObjectPtr<UWorld>(
        FSoftObjectPath("/CesiumForUnreal/Tests/Maps/SingleCube.SingleCube")));
    pLevelComponent1 =
        Cast<UCesiumSubLevelComponent>(pSubLevel1->AddComponentByClass(
            UCesiumSubLevelComponent::StaticClass(),
            false,
            FTransform::Identity,
            false));
    Track(pLevelComponent1);
    pLevelComponent1->SetOriginLongitudeLatitudeHeight(
        FVector(10.0, 20.0, 1000.0));
    pSubLevel1->AddInstanceComponent(pLevelComponent1);

    pSubLevel2 = pWorld->SpawnActor<ALevelInstance>();
    Track(pSubLevel2);
    pSubLevel2->SetWorldAsset(TSoftObjectPtr<UWorld>(FSoftObjectPath(
        "/CesiumForUnreal/Tests/Maps/ConeAndCylinder.ConeAndCylinder")));
    pLevelComponent2 =
        Cast<UCesiumSubLevelComponent>(pSubLevel2->AddComponentByClass(
            UCesiumSubLevelComponent::StaticClass(),
            false,
            FTransform::Identity,
            false));
    Track(pLevelComponent2);
    pLevelComponent2->SetOriginLongitudeLatitudeHeight(
        FVector(-25.0, 15.0, -5000.0));
    pSubLevel2->AddInstanceComponent(pLevelComponent2);

    pSubLevel1->LoadLevelInstance();
    pSubLevel2->LoadLevelInstance();

    pGeoreference = nullptr;
    for (TActorIterator<ACesiumGeoreference> it(pWorld); it; ++it) {
      pGeoreference = *it;
    }

    Track(pGeoreference);

    pPawn = pWorld->SpawnActor<AGlobeAwareDefaultPawn>();
    Track(pPawn);
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
          WaitFor(done, GEditor->PlayWorld, 5.0f, [this]() {
            return !FindInPlay(pSubLevel1)->IsLoaded() &&
                   !FindInPlay(pSubLevel2)->IsLoaded();
          });
        });
    BeforeEach(EAsyncExecution::TaskGraphMainThread, [this]() {
      TestFalse("pSubLevel1 is loaded", FindInPlay(pSubLevel1)->IsLoaded());
      TestFalse("pSubLevel2 is loaded", FindInPlay(pSubLevel2)->IsLoaded());

      // Move the player within range of the first sub-level
      UCesiumSubLevelComponent* pPlayLevelComponent1 =
          FindInPlay(pLevelComponent1);
      FVector origin1 = FindInPlay(pGeoreference)
                            ->TransformLongitudeLatitudeHeightToUnreal(FVector(
                                pPlayLevelComponent1->GetOriginLongitude(),
                                pPlayLevelComponent1->GetOriginLatitude(),
                                pPlayLevelComponent1->GetOriginHeight()));
      FindInPlay(pPawn)->SetActorLocation(origin1);
    });
    LatentBeforeEach(
        EAsyncExecution::TaskGraphMainThread,
        [this](const FDoneDelegate& done) {
          WaitFor(done, GEditor->PlayWorld, 5.0f, [this]() {
            return FindInPlay(pSubLevel1)->IsLoaded();
          });
        });
    It("", EAsyncExecution::TaskGraphMainThread, [this]() {
      TestTrue("pSubLevel1 is loaded", FindInPlay(pSubLevel1)->IsLoaded());
      TestFalse("pSubLevel2 is loaded", FindInPlay(pSubLevel2)->IsLoaded());
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
          WaitFor(done, GEditor->PlayWorld, 5.0f, [this]() {
            return !FindInPlay(pSubLevel1)->IsLoaded() &&
                   !FindInPlay(pSubLevel2)->IsLoaded();
          });
        });
    BeforeEach(EAsyncExecution::TaskGraphMainThread, [this]() {
      TestFalse("pSubLevel1 is loaded", FindInPlay(pSubLevel1)->IsLoaded());
      TestFalse("pSubLevel2 is loaded", FindInPlay(pSubLevel2)->IsLoaded());

      // Move the player within range of the first sub-level
      UCesiumSubLevelComponent* pPlayLevelComponent1 =
          FindInPlay(pLevelComponent1);
      FVector origin1 = FindInPlay(pGeoreference)
                            ->TransformLongitudeLatitudeHeightToUnreal(FVector(
                                pPlayLevelComponent1->GetOriginLongitude(),
                                pPlayLevelComponent1->GetOriginLatitude(),
                                pPlayLevelComponent1->GetOriginHeight()));
      FindInPlay(pPawn)->SetActorLocation(origin1);
    });
    LatentBeforeEach(
        EAsyncExecution::TaskGraphMainThread,
        [this](const FDoneDelegate& done) {
          // Wait for the level to load.
          WaitFor(done, GEditor->PlayWorld, 5.0f, [this]() {
            return FindInPlay(pSubLevel1)->IsLoaded();
          });
        });
    BeforeEach(EAsyncExecution::TaskGraphMainThread, [this]() {
      // Move the player outside the sub-level, triggering an unload.
      UCesiumSubLevelComponent* pPlayLevelComponent1 =
          FindInPlay(pLevelComponent1);
      FVector origin1 =
          FindInPlay(pGeoreference)
              ->TransformLongitudeLatitudeHeightToUnreal(FVector(
                  pPlayLevelComponent1->GetOriginLongitude(),
                  pPlayLevelComponent1->GetOriginLatitude(),
                  pPlayLevelComponent1->GetOriginHeight() + 100000.0));
      FindInPlay(pPawn)->SetActorLocation(origin1);
    });
    BeforeEach(EAsyncExecution::TaskGraphMainThread, [this]() {
      // Without waiting for the level to finish unloading, move the player back
      // inside it.
      UCesiumSubLevelComponent* pPlayLevelComponent1 =
          FindInPlay(pLevelComponent1);
      FVector origin1 = FindInPlay(pGeoreference)
                            ->TransformLongitudeLatitudeHeightToUnreal(FVector(
                                pPlayLevelComponent1->GetOriginLongitude(),
                                pPlayLevelComponent1->GetOriginLatitude(),
                                pPlayLevelComponent1->GetOriginHeight()));
      FindInPlay(pPawn)->SetActorLocation(origin1);
    });
    LatentBeforeEach(
        EAsyncExecution::TaskGraphMainThread,
        [this](const FDoneDelegate& done) {
          // Wait for the level to load.
          WaitFor(done, GEditor->PlayWorld, 5.0f, [this]() {
            return FindInPlay(pSubLevel1)->IsLoaded();
          });
        });
    It("", EAsyncExecution::TaskGraphMainThread, [this]() {
      TestTrue("pSubLevel1 is loaded", FindInPlay(pSubLevel1)->IsLoaded());
      TestFalse("pSubLevel2 is loaded", FindInPlay(pSubLevel2)->IsLoaded());
    });
    AfterEach(EAsyncExecution::TaskGraphMainThread, [this]() {
      GEditor->RequestEndPlayMap();
    });
  });
}

#endif // #if WITH_EDITOR
