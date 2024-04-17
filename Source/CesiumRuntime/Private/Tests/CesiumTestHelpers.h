// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumRuntime.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Math/MathFwd.h"
#include "Misc/AutomationTest.h"
#include "TimerManager.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

class UWorld;
class ACesiumGeoreference;

namespace CesiumTestHelpers {

UWorld* getGlobalWorldContext();

/// <summary>
/// Verify that two rotations (expressed relative to two different
/// georeferences) are equivalent by using them to rotate the principal axis
/// vectors and then transforming those vectors to ECEF. The ECEF vectors should
/// be the same in both cases.
/// </summary>
void TestRotatorsAreEquivalent(
    FAutomationTestBase* pSpec,
    ACesiumGeoreference* pGeoreferenceExpected,
    const FRotator& rotatorExpected,
    ACesiumGeoreference* pGeoreferenceActual,
    const FRotator& rotatorActual);

template <typename T>
void waitForImpl(
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
          waitForImpl<T>(done, pWorld, std::move(condition), timerHandle);
        });
  }
}

/// <summary>
/// Waits for a provided lambda function to become true, ticking through render
/// frames in the meantime. If the timeout elapses before the condition becomes
/// true, an error is logged (which will cause a test failure) and the done
/// delegate is invoked anyway.
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="done">The done delegate provided by a LatentIt or
/// LatentBeforeEach. It will be invoked when the condition is true or when the
/// timeout elapses.</param>
/// <param name="pWorld">The world in which to check the condition.</param>
/// <param name="timeoutSeconds">The maximum time to wait for the condition to
/// become true.</param>
/// <param name="condition">A lambda that is invoked each
/// frame. If this function returns false, waiting continues.</param>
template <typename T>
void waitFor(
    const FDoneDelegate& done,
    UWorld* pWorld,
    float timeoutSeconds,
    T&& condition) {
  FTimerHandle timerHandle;
  pWorld->GetTimerManager().SetTimer(timerHandle, timeoutSeconds, false);
  waitForImpl<T>(done, pWorld, std::forward<T>(condition), timerHandle);
}

void waitForNextFrame(
    const FDoneDelegate& done,
    UWorld* pWorld,
    float timeoutSeconds);

/// <summary>
/// Gets the first Actor that has a given tag.
/// </summary>
/// <typeparam name="T">The Actor type.</typeparam>
/// <param name="pWorld">The world in which to search for the Actor.</param>
/// <param name="tag">The tag to look for.</param>
/// <returns>The Actor, or nullptr if it does not exist.</returns>
template <typename T> T* getActorWithTag(UWorld* pWorld, const FName& tag) {
  TArray<AActor*> temp;
  UGameplayStatics::GetAllActorsWithTag(pWorld, tag, temp);
  if (temp.Num() < 1)
    return nullptr;

  return Cast<T>(temp[0]);
}

/// <summary>
/// Gets the first ActorComponent that has a given tag.
/// </summary>
/// <typeparam name="T">The ActorComponent type.</typeparam>
/// <param name="pOwner">The Actor whose components to search.</param>
/// <param name="tag">The tag to look for.</param>
/// <returns>The ActorComponent, or nullptr if it does not exist.</returns>
template <typename T> T* getComponentWithTag(AActor* pOwner, const FName& tag) {
  TArray<UActorComponent*> components =
      pOwner->GetComponentsByTag(T::StaticClass(), tag);
  if (components.Num() < 1)
    return nullptr;

  return Cast<T>(components[0]);
}

/// <summary>
/// Gets a tag that can be used to uniquely identify a given Actor.
/// </summary>
/// <param name="pActor">The actor.</param>
/// <returns>The unique tag.</returns>
FName getUniqueTag(AActor* pActor);

/// <summary>
/// Gets a tag that can be used to uniquely identify a given ActorComponent.
/// </summary>
/// <param name="pActor">The actor.</param>
/// <returns>The unique tag.</returns>
FName getUniqueTag(UActorComponent* pComponent);

#if WITH_EDITOR

/// <summary>
/// Tracks a provided Edit-mode Actor, so the equivalent object can later be
/// found in Play mode.
/// </summary>
/// <param name="pEditorActor">The Actor in the Editor world to track.</param>
void trackForPlay(AActor* pEditorActor);

/// <summary>
/// Tracks a provided Edit-mode ActorComponent, so the equivalent object can
/// later be found in Play mode.
/// </summary>
/// <param name="pEditorComponent">The ActorComponent in the Editor world to
/// track.</param>
void trackForPlay(UActorComponent* pEditorComponent);

/// <summary>
/// Finds a Play-mode object equivalent to a given Editor-mode one that was
/// previously tracked with <see cref="TrackForPlay" />. This works on instances
/// derived from AActor and UActorComponent.
/// </summary>
/// <typeparam name="T">The type of the object to find.</typeparam>
/// <param name="pEditorObject">The Editor object for which to find a Play-mode
/// equivalent.</param>
/// <returns>The play mode equivalent, or nullptr is one is not found.</returns>
template <typename T> T* findInPlay(T* pEditorObject) {
  if (!IsValid(pEditorObject))
    return nullptr;

  UWorld* pWorld = GEditor->PlayWorld;
  if constexpr (std::is_base_of_v<AActor, T>) {
    return getActorWithTag<T>(pWorld, getUniqueTag(pEditorObject));
  } else if constexpr (std::is_base_of_v<UActorComponent, T>) {
    AActor* pEditorOwner = pEditorObject->GetOwner();
    if (!pEditorOwner)
      return nullptr;
    AActor* pPlayOwner = findInPlay(pEditorOwner);
    if (!pPlayOwner)
      return nullptr;
    return getComponentWithTag<T>(pPlayOwner, getUniqueTag(pEditorObject));
  }

  return nullptr;
}

/// <summary>
/// Finds a Play-mode object equivalent to a given Editor-mode one that was
/// previously tracked with <see cref="TrackForPlay" />. This works on instances
/// derived from AActor and UActorComponent.
/// </summary>
/// <typeparam name="T">The type of the object to find.</typeparam>
/// <param name="pEditorObject">The Editor object for which to find a Play-mode
/// equivalent.</param>
/// <returns>The play mode equivalent, or nullptr is one is
/// not found.</returns>
template <typename T> T* findInPlay(TObjectPtr<T> pEditorObject) {
  return findInPlay<T>(pEditorObject.Get());
}

#endif // #if WITH_EDITOR

} // namespace CesiumTestHelpers
