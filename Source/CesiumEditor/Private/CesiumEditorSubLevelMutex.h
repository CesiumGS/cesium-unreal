// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Delegates/IDelegateInstance.h"

class UActorComponent;

/**
 * Ensures that only a single ALevelInstance with a UCesiumSubLevelComponent is
 * visible in the Editor at any given time. It works by subscribing to the
 * static MarkRenderStateDirtyEvent on UActorComponent, which is raised when the
 * user toggles the visibility of an Actor in the Editor.
 */
class CesiumEditorSubLevelMutex {
public:
  CesiumEditorSubLevelMutex();
  ~CesiumEditorSubLevelMutex();

private:
  void OnMarkRenderStateDirty(UActorComponent& component);

  FDelegateHandle _subscription;
};
