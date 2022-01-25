
#pragma once

#include "Math/Rotator.h"
#include "Math/Vector.h"
#include "Math/Vector2D.h"
#include "UObject/ObjectMacros.h"

#include "Cesium3DTilesSelection/ViewState.h"

#include "CesiumCamera.generated.h"

USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumCamera {
  GENERATED_USTRUCT_BODY()

public:
  UPROPERTY(BlueprintReadWrite)
  FVector2D ViewportSize;

  UPROPERTY(BlueprintReadWrite)
  FVector Location;

  UPROPERTY(BlueprintReadWrite)
  FRotator Rotation;

  UPROPERTY(BlueprintReadWrite)
  float FieldOfViewDegrees;

  // The aspect ratio may be different from the one implied by the ViewportSize
  // and black bars are added as needed in order to achieve this aspect ratio
  // within a larger viewport.
  UPROPERTY(BlueprintReadWrite)
  float AspectRatio = 0.0f;

  FCesiumCamera();

  FCesiumCamera(
      const FVector2D& ViewportSize,
      const FVector& Location,
      const FRotator& Rotation,
      float FieldOfViewDegrees);

  FCesiumCamera(
      const FVector2D& ViewportSize,
      const FVector& Location,
      const FRotator& Rotation,
      float FieldOfViewDegrees,
      float AspectRatio);

  Cesium3DTilesSelection::ViewState
  createViewState(const glm::dmat4& unrealWorldToTileset) const;
};
