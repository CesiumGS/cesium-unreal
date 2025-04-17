// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "Components/SceneCaptureComponent2D.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "CesiumSceneCaptureComponent2D.generated.h"

/**
 * Used to capture a 'snapshot' of the scene from a single plane and feed it to
 * a render target.
 *
 * This is identical to Unreal Engine's built-in
 * `USceneCaptureCaptureComponent2D` except that it overrides `GetViewOwner` to
 * returning `GetOwner`. This allows the capture component to be used as a view
 * of a `Cesium3DTileset`.
 */
UCLASS(
    hidecategories = (Collision, Object, Physics, SceneComponent),
    ClassGroup = Rendering,
    editinlinenew,
    meta = (BlueprintSpawnableComponent),
    MinimalAPI)
class UCesiumSceneCaptureComponent2D : public USceneCaptureComponent2D {
  GENERATED_UCLASS_BODY()

public:
  const AActor* GetViewOwner() const override;
};
