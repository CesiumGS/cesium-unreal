// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#include "CesiumSceneCaptureComponent2D.h"

UCesiumSceneCaptureComponent2D::UCesiumSceneCaptureComponent2D(
    const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {}

const AActor* UCesiumSceneCaptureComponent2D::GetViewOwner() const {
  return this->GetOwner();
}
