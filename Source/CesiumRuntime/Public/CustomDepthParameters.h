// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include <Components/PrimitiveComponent.h>
#include <CoreMinimal.h>

#include "CustomDepthParameters.generated.h"

USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCustomDepthParameters {
  GENERATED_USTRUCT_BODY()

public:
  /** If true, this component will be rendered in the CustomDepth pass (usually
   * used for outlines) */
  UPROPERTY(
      EditAnywhere,
      AdvancedDisplay,
      BlueprintReadOnly,
      Category = Rendering,
      meta = (DisplayName = "Render CustomDepth Pass"))
  bool RenderCustomDepth = false;

  /** Mask used for stencil buffer writes. */
  UPROPERTY(
      EditAnywhere,
      AdvancedDisplay,
      BlueprintReadOnly,
      Category = "Rendering",
      meta = (EditCondition = "RenderCustomDepth"))
  ERendererStencilMask CustomDepthStencilWriteMask =
      ERendererStencilMask::ERSM_Default;

  /** Optionally write this 0-255 value to the stencil buffer in CustomDepth
   * pass (Requires project setting or r.CustomDepth == 3) */
  UPROPERTY(
      EditAnywhere,
      AdvancedDisplay,
      BlueprintReadOnly,
      Category = Rendering,
      meta = (UIMin = "0", UIMax = "255", EditCondition = "RenderCustomDepth"))
  int32 CustomDepthStencilValue = 0;

  bool operator==(const FCustomDepthParameters& other) const {
    return RenderCustomDepth == other.RenderCustomDepth &&
           CustomDepthStencilWriteMask == other.CustomDepthStencilWriteMask &&
           CustomDepthStencilValue == other.CustomDepthStencilValue;
  }

  bool operator!=(const FCustomDepthParameters& other) const {
    return !(*this == other);
  }
};
