// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumTestHelpers.h"
#include "Engine/Engine.h"

namespace CesiumTestHelpers {

UWorld* getGlobalWorldContext() {
  const TIndirectArray<FWorldContext>& worldContexts =
      GEngine->GetWorldContexts();
  FWorldContext firstWorldContext = worldContexts[0];
  return firstWorldContext.World();
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
