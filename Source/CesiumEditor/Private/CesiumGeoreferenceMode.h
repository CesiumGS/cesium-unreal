// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "EdMode.h"
#include "Editor.h"

class FCesiumGeoreferenceMode : public FEdMode {
public:
  const static FEditorModeID EM_CesiumGeoreferenceMode;

  virtual void Enter() override;
  virtual void Exit() override;

  virtual bool InputDelta(
      FEditorViewportClient* InViewportClient,
      FViewport* InViewport,
      FVector& InDrag,
      FRotator& InRot,
      FVector& InScale) override;

  virtual bool InputKey(
      FEditorViewportClient* InViewportClient,
      FViewport* InViewport,
      FKey Key,
      EInputEvent Event) override;

private:
  ACesiumGeoreference* Georeference;
};