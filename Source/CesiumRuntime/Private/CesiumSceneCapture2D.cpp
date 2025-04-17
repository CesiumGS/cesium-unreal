// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#include "CesiumSceneCapture2D.h"
#include "CesiumSceneCaptureComponent2D.h"

ACesiumSceneCapture2D::ACesiumSceneCapture2D(
    const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
  CaptureComponent2D = CreateDefaultSubobject<UCesiumSceneCaptureComponent2D>(
      TEXT("NewSceneCaptureComponent2D"));
  CaptureComponent2D->SetupAttachment(RootComponent);
}

void ACesiumSceneCapture2D::OnInterpToggle(bool bEnable) {
  CaptureComponent2D->SetVisibility(bEnable);
}

void ACesiumSceneCapture2D::CalcCamera(
    float DeltaTime,
    FMinimalViewInfo& OutMinimalViewInfo) {
  if (UCesiumSceneCaptureComponent2D* SceneCaptureComponent =
          GetCaptureComponent2D()) {
    SceneCaptureComponent->GetCameraView(DeltaTime, OutMinimalViewInfo);
  } else {
    Super::CalcCamera(DeltaTime, OutMinimalViewInfo);
  }
}
