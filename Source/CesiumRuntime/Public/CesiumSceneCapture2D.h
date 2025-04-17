// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "Engine/SceneCapture.h"
#include "UObject/ObjectMacros.h"
#include "CesiumSceneCapture2D.generated.h"

UCLASS(hidecategories = (Collision, Material, Attachment, Actor), MinimalAPI)
class ACesiumSceneCapture2D : public ASceneCapture {
  GENERATED_UCLASS_BODY()

private:
  /** Scene capture component. */
  UPROPERTY(
      Category = Cesium,
      VisibleAnywhere,
      BlueprintReadOnly,
      meta = (AllowPrivateAccess = "true"))
  TObjectPtr<class UCesiumSceneCaptureComponent2D> CaptureComponent2D;

public:
  UFUNCTION(BlueprintCallable, Category = "Rendering")
  void OnInterpToggle(bool bEnable);

  CESIUMRUNTIME_API virtual void CalcCamera(
      float DeltaTime,
      struct FMinimalViewInfo& OutMinimalViewInfo) override;

  /** Returns CaptureComponent2D subobject **/
  class UCesiumSceneCaptureComponent2D* GetCaptureComponent2D() const {
    return CaptureComponent2D;
  }
};
