// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DefaultPawn.h"
#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>
#include <vector>
#include "GlobeAwareDefaultPawn.generated.h"

class ACesiumGeoreference;
class UCesiumGlobeAnchorComponent;
class UCurveFloat;

/**
 * This pawn can be used to easily move around the globe while maintaining a
 * sensible orientation. As the pawn moves across the horizon, it automatically
 * changes its own up direction such that the world always looks right-side up.
 */
UCLASS()
class CESIUMRUNTIME_API AGlobeAwareDefaultPawn : public ADefaultPawn {
  GENERATED_BODY()

public:
  AGlobeAwareDefaultPawn();

  /**
   * Input callback to move forward in local space (or backward if Val is
   * negative).
   * @param Val Amount of movement in the forward direction (or backward if
   * negative).
   * @see APawn::AddMovementInput()
   */
  virtual void MoveForward(float Val) override;

  /**
   * Input callback to strafe right in local space (or left if Val is negative).
   * @param Val Amount of movement in the right direction (or left if negative).
   * @see APawn::AddMovementInput()
   */
  virtual void MoveRight(float Val) override;

  /**
   * Input callback to move up in world space (or down if Val is negative).
   * @param Val Amount of movement in the world up direction (or down if
   * negative).
   * @see APawn::AddMovementInput()
   */
  virtual void MoveUp_World(float Val) override;

  /**
   * Gets the absolute rotation of the camera view from the Unreal world.
   */
  virtual FRotator GetViewRotation() const override;

  /**
   * Gets the rotation of the aim direction, which is the same as the View
   * Rotation.
   */
  virtual FRotator GetBaseAimRotation() const override;

  /**
   * This curve dictates what percentage of the max altitude the pawn should
   * take at a given time on the curve. This curve must be kept in the 0 to
   * 1 range on both axes. The {@see FlyToMaximumAltitudeCurve} dictates the
   * actual max altitude at each point along the curve.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  UCurveFloat* FlyToAltitudeProfileCurve;

  /**
   * This curve is used to determine the progress percentage for all the other
   * curves. This allows us to accelerate and deaccelerate as wanted throughout
   * the curve.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  UCurveFloat* FlyToProgressCurve;

  /**
   * This curve dictates the maximum altitude at each point along the curve.
   * This can be used in conjunction with the {@see FlyToAltitudeProfileCurve}
   * to allow the pawn to take some altitude during the flight.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  UCurveFloat* FlyToMaximumAltitudeCurve;

  /**
   * The length in seconds that the flight should last.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium", meta = (ClampMin = 0.0))
  double FlyToDuration = 5.0;

  /**
   * The granularity in degrees with which keypoints should be generated for
   * the flight interpolation.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium", meta = (ClampMin = 0.0))
  double FlyToGranularityDegrees = 0.01;

  /**
   * Begin a smooth camera flight to the given Earth-Centered, Earth-Fixed
   * (ECEF) destination such that the camera ends at the specified yaw and
   * pitch. The characteristics of the flight can be configured with
   * {@see FlyToAltitudeProfileCurve}, {@see FlyToProgressCurve},
   * {@see FlyToMaximumAltitudeCurve}, {@see FlyToDuration}, and
   * {@see FlyToGranularityDegrees}.
   */
  void FlyToLocationECEF(
      const glm::dvec3& ECEFDestination,
      float YawAtDestination,
      float PitchAtDestination,
      bool CanInterruptByMoving);

  /**
   * Begin a smooth camera flight to the given Earth-Centered, Earth-Fixed
   * (ECEF) destination such that the camera ends at the specified yaw and
   * pitch. The characteristics of the flight can be configured with
   * {@see FlyToAltitudeProfileCurve}, {@see FlyToProgressCurve},
   * {@see FlyToMaximumAltitudeCurve}, {@see FlyToDuration}, and
   * {@see FlyToGranularityDegrees}.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void InaccurateFlyToLocationECEF(
      const FVector& ECEFDestination,
      float YawAtDestination,
      float PitchAtDestination,
      bool CanInterruptByMoving);

  /**
   * Begin a smooth camera flight to the given WGS84 longitude in degrees (x),
   * latitude in degrees (y), and height in meters (z) such that the camera
   * ends at the given yaw and pitch. The characteristics of the flight can be
   * configured with {@see FlyToAltitudeProfileCurve},
   * {@see FlyToProgressCurve}, {@see FlyToMaximumAltitudeCurve},
   * {@see FlyToDuration}, and {@see FlyToGranularityDegrees}.
   */
  void FlyToLocationLongitudeLatitudeHeight(
      const glm::dvec3& LongitudeLatitudeHeightDestination,
      float YawAtDestination,
      float PitchAtDestination,
      bool CanInterruptByMoving);

  /**
   * Begin a smooth camera flight to the given WGS84 longitude in degrees (x),
   * latitude in degrees (y), and height in meters (z) such that the camera
   * ends at the given yaw and pitch. The characteristics of the flight can be
   * configured with {@see FlyToAltitudeProfileCurve},
   * {@see FlyToProgressCurve}, {@see FlyToMaximumAltitudeCurve},
   * {@see FlyToDuration}, and {@see FlyToGranularityDegrees}.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void InaccurateFlyToLocationLongitudeLatitudeHeight(
      const FVector& LongitudeLatitudeHeightDestination,
      float YawAtDestination,
      float PitchAtDestination,
      bool CanInterruptByMoving);

  virtual bool ShouldTickIfViewportsOnly() const override;
  virtual void Tick(float DeltaSeconds) override;
  virtual void PostLoad() override;

protected:
  /**
   * THIS PROPERTY IS DEPRECATED.
   *
   * Get the Georeference instance from the Globe Anchor Component instead.
   */
  UPROPERTY(
      BlueprintReadOnly,
      Category = "Cesium",
      BlueprintGetter = GetGeoreference,
      Meta =
          (DeprecatedProperty,
           DeprecationMessage =
               "Get the Georeference instance from the Globe Anchor Component instead."))
  ACesiumGeoreference* Georeference_DEPRECATED;

  /**
   * Gets the Georeference Actor associated with this instance. It is obtained
   * from the Globe Anchor Component.
   */
  UFUNCTION(BlueprintGetter, Category = "Cesium")
  ACesiumGeoreference* GetGeoreference() const;

  /**
   * The Globe Anchor Component that precisely ties this Pawn to the Globe.
   */
  UPROPERTY(
      VisibleAnywhere,
      BlueprintReadOnly,
      Category = "Cesium" /*,
      Meta = (ShowOnlyInnerProperties = true) */)
  UCesiumGlobeAnchorComponent* GlobeAnchor;

private:
  void _moveAlongViewAxis(EAxis::Type axis, float Val);
  void _moveAlongVector(const FVector& axis, float Val);
  void _interruptFlight();

  /**
   * @brief Advance the camera flight based on the given time delta.
   *
   * NOTE: This function requires the Georefence to be valid. If it
   * is not valid, then this function will do nothing.
   *
   * The given delta will be added to the _currentFlyTime, and the position
   * and orientation will be computed by interpolating the _keypoints
   * and _flyToSourceRotation/_flyToDestinationRotation  based on this time.
   *
   * The position will be set as the SetECEFCameraLocation, and the
   * orientation will be assigned GetController()->SetControlRotation.
   *
   * @param DeltaSeconds The time delta, in seconds
   */
  void _handleFlightStep(float DeltaSeconds);

  // helper variables for FlyToLocation
  bool _bFlyingToLocation = false;
  bool _bCanInterruptFlight = false;
  double _currentFlyTime = 0.0;
  FQuat _flyToSourceRotation;
  FQuat _flyToDestinationRotation;

  std::vector<glm::dvec3> _keypoints;
};
