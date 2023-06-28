// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumTestHelpers.h"
#include "Engine/Engine.h"

UWorld* CesiumTestHelpers::getGlobalWorldContext() {
  const TIndirectArray<FWorldContext>& worldContexts =
      GEngine->GetWorldContexts();
  FWorldContext firstWorldContext = worldContexts[0];
  return firstWorldContext.World();
}
