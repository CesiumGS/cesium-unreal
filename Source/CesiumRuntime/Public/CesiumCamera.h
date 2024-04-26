// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Math/Rotator.h"
#include "Math/Vector.h"
#include "Math/Vector2D.h"
#include "UObject/ObjectMacros.h"

#include "Cesium3DTilesSelection/ViewState.h"

#include "CesiumCamera.generated.h"

/**
 * @brief A camera description that {@link Cesium3DTileset}s can use to decide
 * what tiles need to be loaded to sufficiently cover the camera view.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumCamera {
  GENERATED_USTRUCT_BODY()

public:
  /**
   * @brief The pixel dimensions of the viewport.
   */
  UPROPERTY(BlueprintReadWrite, Category = "Cesium")
  FVector2D ViewportSize;

  /**
   * @brief The Unreal location of the camera.
   */
  UPROPERTY(BlueprintReadWrite, Category = "Cesium")
  FVector Location;

  /**
   * @brief The Unreal rotation of the camera.
   */
  UPROPERTY(BlueprintReadWrite, Category = "Cesium")
  FRotator Rotation;

  /**
   * @brief The horizontal field of view of the camera in degrees.
   */
  UPROPERTY(BlueprintReadWrite, Category = "Cesium")
  double FieldOfViewDegrees;

  /**
   * @brief The overriden aspect ratio for this camera.
   *
   * When this is 0.0f, use the aspect ratio implied by ViewportSize.
   *
   * This may be different from the aspect ratio implied by the ViewportSize
   * and black bars are added as needed in order to achieve this aspect ratio
   * within a larger viewport.
   */
  UPROPERTY(BlueprintReadWrite, Category = "Cesium")
  double OverrideAspectRatio = 0.0;

  /**
   * @brief Construct an uninitialized FCesiumCamera object.
   */
  FCesiumCamera();

  /**
   * @brief Construct a new FCesiumCamera object.
   *
   * @param ViewportSize The viewport pixel size.
   * @param Location The Unreal location.
   * @param Rotation The Unreal rotation.
   * @param FieldOfViewDegrees The horizontal field of view in degrees.
   */
  FCesiumCamera(
      const FVector2D& ViewportSize,
      const FVector& Location,
      const FRotator& Rotation,
      double FieldOfViewDegrees);

  /**
   * @brief Construct a new FCesiumCamera object.
   *
   * @param ViewportSize The viewport pixel size.
   * @param Location The Unreal location.
   * @param Rotation The Unreal rotation.
   * @param FieldOfViewDegrees The horizontal field of view in degrees.
   * @param OverrideAspectRatio The overriden aspect ratio.
   */
  FCesiumCamera(
      const FVector2D& ViewportSize,
      const FVector& Location,
      const FRotator& Rotation,
      double FieldOfViewDegrees,
      double OverrideAspectRatio);
};
