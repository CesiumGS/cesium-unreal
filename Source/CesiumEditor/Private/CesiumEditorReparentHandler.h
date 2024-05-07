// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Delegates/IDelegateInstance.h"

class AActor;

/**
 * Detects when Actors are reparented in the Editor by subscribing to
 * GEngine::OnLevelActorAttached and handling it appropriately. For example,
 * when a Cesium3DTileset's parent changes, we need to re-resolve its
 * CesiumGeoreference.
 */
class CesiumEditorReparentHandler {
public:
  CesiumEditorReparentHandler();
  ~CesiumEditorReparentHandler();

private:
  void OnLevelActorAttached(AActor* Actor, const AActor* Parent);

  FDelegateHandle _subscription;
};
