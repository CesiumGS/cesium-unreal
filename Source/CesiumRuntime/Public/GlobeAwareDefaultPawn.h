// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DefaultPawn.h"
#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>
#include <vector>
#include "GlobeAwareDefaultPawn.generated.h"

class ACesiumGeoreference;
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
   * The actor controlling how this camera's location in the Cesium world
   * relates to the coordinate system in this Unreal Engine level.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cesium")
  ACesiumGeoreference* Georeference;

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
   * Get the view rotation of the Pawn (direction they are looking,
   * normally Controller->ControlRotation).
   *
   * This is overridden to include the rotation that is determined
   * by the georeference East-North-Up to Unreal translation.
   *
   * @return The view rotation of the Pawn.
   */
  virtual FRotator GetViewRotation() const override;

  /**
   * Return the aim rotation for the Pawn.
   *
   * This is overridden to return the same as GetViewRotation.
   *
   * @return The aim rotation of the Pawn.
   */
  virtual FRotator GetBaseAimRotation() const override;

  /**
   * Get the pawn Camera location in Earth-Centered, Earth-Fixed (ECEF)
   * coordinates. This is the position of the actor, excluding the
   * BaseEyeHeight.
   */
  glm::dvec3 GetECEFCameraLocation() const;

  /**
   * Set the pawn Camera location from Earth-Centered, Earth-Fixed (ECEF)
   * coordinates. This is the position of the actor, excluding the
   * BaseEyeHeight.
   */
  void SetECEFCameraLocation(const glm::dvec3& ECEF);

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

  UFUNCTION()
  void HandleGeoreferenceUpdated();

  virtual bool ShouldTickIfViewportsOnly() const override;
  virtual void Tick(float DeltaSeconds) override;

protected:
  /**
   * Called after the C++ constructor and after the properties have
   * been initialized, including those loaded from config.
   */
  void PostInitProperties() override;

  virtual void PostActorCreated() override;
  virtual void PostLoad() override;

  virtual void BeginPlay() override;

#if WITH_EDITOR

  /**
   * This is called when a property is about to be modified externally.
   *
   * When the georeference is about to be modified, then this will
   * remove the `HandleGeoreferenceUpdated` callback from the
   * `OnGeoreferenceUpdated` delegate of the current georeference.
   */
  void PreEditChange(FProperty* PropertyThatWillChange);

  /**
   * Called when a property on this object has been modified externally
   *
   * This is called every time that a value is modified in the editor UI.
   *
   * When the georeference has been be modified, then this will
   * attach the `HandleGeoreferenceUpdated` callback to the
   * `OnGeoreferenceUpdated` delegate of the new georeference.
   */
  virtual void
  PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
  void _interruptFlight();
  void _initGeoreference();

  /**
   * Computes the normal of the ellipsoid at the given location,
   * in unreal coordinates.
   */
  FVector _computeEllipsoidNormalUnreal(const FVector& location);

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

  /**
   * The x-coordinate of the position of this actor, in ECEF coordinates,
   * corresponding to GetActorLocation().X
   */
  UPROPERTY()
  double currentEcefX;

  /**
   * The y-coordinate of the position of this actor, in ECEF coordinates,
   * corresponding to GetActorLocation().Y
   */
  UPROPERTY()
  double currentEcefY;

  /**
   * The z-coordinate of the position of this actor, in ECEF coordinates,
   * corresponding to GetActorLocation().Z
   */
  UPROPERTY()
  double currentEcefZ;

  // helper variables for FlyToLocation
  bool _bFlyingToLocation = false;
  bool _bCanInterruptFlight = false;
  double _currentFlyTime = 0.0;
  FRotator _flyToSourceRotation;
  FRotator _flyToDestinationRotation;

  std::vector<glm::dvec3> _keypoints;
};
