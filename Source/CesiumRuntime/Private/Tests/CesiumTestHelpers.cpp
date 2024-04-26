// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumTestHelpers.h"
#include "CesiumGeoreference.h"
#include "Engine/Engine.h"

namespace CesiumTestHelpers {

UWorld* getGlobalWorldContext() {
  const TIndirectArray<FWorldContext>& worldContexts =
      GEngine->GetWorldContexts();
  FWorldContext firstWorldContext = worldContexts[0];
  return firstWorldContext.World();
}

void TestRotatorsAreEquivalent(
    FAutomationTestBase* pSpec,
    ACesiumGeoreference* pGeoreferenceExpected,
    const FRotator& rotatorExpected,
    ACesiumGeoreference* pGeoreferenceActual,
    const FRotator& rotatorActual) {
  FVector xEcefExpected =
      pGeoreferenceExpected->TransformUnrealDirectionToEarthCenteredEarthFixed(
          rotatorExpected.RotateVector(FVector::XAxisVector));
  FVector yEcefExpected =
      pGeoreferenceExpected->TransformUnrealDirectionToEarthCenteredEarthFixed(
          rotatorExpected.RotateVector(FVector::YAxisVector));
  FVector zEcefExpected =
      pGeoreferenceExpected->TransformUnrealDirectionToEarthCenteredEarthFixed(
          rotatorExpected.RotateVector(FVector::ZAxisVector));

  FVector xEcefActual =
      pGeoreferenceActual->TransformUnrealDirectionToEarthCenteredEarthFixed(
          rotatorActual.RotateVector(FVector::XAxisVector));
  FVector yEcefActual =
      pGeoreferenceActual->TransformUnrealDirectionToEarthCenteredEarthFixed(
          rotatorActual.RotateVector(FVector::YAxisVector));
  FVector zEcefActual =
      pGeoreferenceActual->TransformUnrealDirectionToEarthCenteredEarthFixed(
          rotatorActual.RotateVector(FVector::ZAxisVector));

  pSpec->TestEqual("xEcefActual", xEcefActual, xEcefExpected);
  pSpec->TestEqual("yEcefActual", yEcefActual, yEcefExpected);
  pSpec->TestEqual("zEcefActual", zEcefActual, zEcefExpected);
}

void waitForNextFrame(
    const FDoneDelegate& done,
    UWorld* pWorld,
    float timeoutSeconds) {
  uint64 start = GFrameCounter;
  waitFor(done, pWorld, timeoutSeconds, [start]() {
    uint64 current = GFrameCounter;
    return current > start;
  });
}

FName getUniqueTag(AActor* pActor) {
  return FName(FString::Printf(TEXT("%lld"), pActor));
}

FName getUniqueTag(UActorComponent* pComponent) {
  return FName(FString::Printf(TEXT("%lld"), pComponent));
}

void trackForPlay(AActor* pEditorActor) {
  pEditorActor->Tags.Add(getUniqueTag(pEditorActor));
}

void trackForPlay(UActorComponent* pEditorComponent) {
  trackForPlay(pEditorComponent->GetOwner());
  pEditorComponent->ComponentTags.Add(getUniqueTag(pEditorComponent));
}

} // namespace CesiumTestHelpers
