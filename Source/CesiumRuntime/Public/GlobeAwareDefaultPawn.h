// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGeospatial/Ellipsoid.h"
#include "CoreMinimal.h"
#include "GameFramework/DefaultPawn.h"
#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>
#include <vector>
#include "GlobeAwareDefaultPawn.generated.h"

class ACesiumGeoreference;
class UCesiumGlobeAnchorComponent;
class UCurveFloat;
class UCesiumFlyToComponent;

/**
 * The delegate for when the pawn finishes flying
 * which is triggered from _handleFlightStep
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCompletedFlight);

/**
 * The delegate for when the pawn's flying is interrupted
 * which is triggered from _interruptFlight
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInterruptedFlight);

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

  UPROPERTY(
      meta =
          (DeprecatedProperty,
           DeprecationMessage =
               "FlyToGranularityDegrees has been deprecated. This property no longer needs to be set."))
  float FlyToGranularityDegrees_DEPRECATED = 0.01f;

  UPROPERTY(
      BlueprintAssignable,
      meta =
          (DeprecatedProperty,
           DeprecationMessage =
               "Use OnFlightComplete on CesiumFlyToComponent instead."))
  FCompletedFlight OnFlightComplete_DEPRECATED;

  UPROPERTY(
      BlueprintAssignable,
      meta =
          (DeprecatedProperty,
           DeprecationMessage =
               "Use OnFlightInterrupted on CesiumFlyToComponent instead."))
  FInterruptedFlight OnFlightInterrupt_DEPRECATED;

  /**
   * Gets the transformation from globe's reference frame to the Unreal world
   * (relative to the floating origin). This is equivalent to calling
   * GetActorTransform on this pawn's attach parent, if it has one. If this pawn
   * does not have an attach parent, an identity transformation is returned.
   */
  UFUNCTION(BlueprintPure, Category = "Cesium")
  const FTransform& GetGlobeToUnrealWorldTransform() const;

  /**
   * Begin a smooth camera flight to the given Earth-Centered, Earth-Fixed
   * (ECEF) destination such that the camera ends at the specified yaw and
   * pitch. The characteristics of the flight can be configured with
   * {@see FlyToAltitudeProfileCurve}, {@see FlyToProgressCurve},
   * {@see FlyToMaximumAltitudeCurve}, and {@see FlyToDuration}
   */
  UFUNCTION(
      BlueprintCallable,
      meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "Use FlyToEarthCenteredEarthFixed on CesiumFlyToComponent instead."))
  void FlyToLocationECEF(
      const FVector& ECEFDestination,
      double YawAtDestination,
      double PitchAtDestination,
      bool CanInterruptByMoving);

  /**
   * Begin a smooth camera flight to the given WGS84 longitude in degrees (x),
   * latitude in degrees (y), and height in meters (z) such that the camera
   * ends at the given yaw and pitch. The characteristics of the flight can be
   * configured with {@see FlyToAltitudeProfileCurve},
   * {@see FlyToProgressCurve}, {@see FlyToMaximumAltitudeCurve},
   * {@see FlyToDuration}, and {@see FlyToGranularityDegrees}.
   */
  UFUNCTION(
      BlueprintCallable,
      meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "Use FlyToLocationLongitudeLatitudeHeight on CesiumFlyToComponent instead."))
  void FlyToLocationLongitudeLatitudeHeight(
      const FVector& LongitudeLatitudeHeightDestination,
      double YawAtDestination,
      double PitchAtDestination,
      bool CanInterruptByMoving);

protected:
  virtual void Serialize(FArchive& Ar) override;
  virtual void PostLoad() override;

  /**
   * THIS PROPERTY IS DEPRECATED.
   *
   * Get the Georeference instance from the Globe Anchor Component instead.
   */
  UPROPERTY(
      Category = "Cesium",
      BlueprintReadOnly,
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
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cesium")
  UCesiumGlobeAnchorComponent* GlobeAnchor;

private:
  UPROPERTY(
      Category = "Cesium",
      BlueprintGetter = GetFlyToProgressCurve_DEPRECATED,
      BlueprintSetter = SetFlyToProgressCurve_DEPRECATED,
      meta =
          (AllowPrivateAccess,
           DeprecatedProperty,
           DeprecationMessage =
               "Use ProgressCurve on CesiumFlyToComponent instead."))
  UCurveFloat* FlyToProgressCurve_DEPRECATED;
  UFUNCTION(BlueprintGetter)
  UCurveFloat* GetFlyToProgressCurve_DEPRECATED() const;
  UFUNCTION(BlueprintSetter)
  void SetFlyToProgressCurve_DEPRECATED(UCurveFloat* NewValue);

  UPROPERTY(
      Category = "Cesium",
      BlueprintGetter = GetFlyToAltitudeProfileCurve_DEPRECATED,
      BlueprintSetter = SetFlyToAltitudeProfileCurve_DEPRECATED,
      meta =
          (AllowPrivateAccess,
           DeprecatedProperty,
           DeprecationMessage =
               "Use HeightPercentageCurve on CesiumFlyToComponent instead."))
  UCurveFloat* FlyToAltitudeProfileCurve_DEPRECATED;
  UFUNCTION(BlueprintGetter)
  UCurveFloat* GetFlyToAltitudeProfileCurve_DEPRECATED() const;
  UFUNCTION(BlueprintSetter)
  void SetFlyToAltitudeProfileCurve_DEPRECATED(UCurveFloat* NewValue);

  UPROPERTY(
      Category = "Cesium",
      BlueprintGetter = GetFlyToMaximumAltitudeCurve_DEPRECATED,
      BlueprintSetter = SetFlyToMaximumAltitudeCurve_DEPRECATED,
      meta =
          (AllowPrivateAccess,
           DeprecatedProperty,
           DeprecationMessage =
               "Use MaximumHeightByDistanceCurve on CesiumFlyToComponent instead."))
  UCurveFloat* FlyToMaximumAltitudeCurve_DEPRECATED;
  UFUNCTION(BlueprintGetter)
  UCurveFloat* GetFlyToMaximumAltitudeCurve_DEPRECATED() const;
  UFUNCTION(BlueprintSetter)
  void SetFlyToMaximumAltitudeCurve_DEPRECATED(UCurveFloat* NewValue);

  UPROPERTY(
      Category = "Cesium",
      BlueprintGetter = GetFlyToDuration_DEPRECATED,
      BlueprintSetter = SetFlyToDuration_DEPRECATED,
      meta =
          (AllowPrivateAccess,
           DeprecatedProperty,
           DeprecationMessage =
               "Use Duration on CesiumFlyToComponent instead."))
  float FlyToDuration_DEPRECATED = 5.0f;
  UFUNCTION(BlueprintGetter)
  float GetFlyToDuration_DEPRECATED() const;
  UFUNCTION(BlueprintSetter)
  void SetFlyToDuration_DEPRECATED(float NewValue);

  void _moveAlongViewAxis(EAxis::Type axis, double Val);
  void _moveAlongVector(const FVector& axis, double Val);

  UFUNCTION()
  void _onFlightComplete();

  UFUNCTION()
  void _onFlightInterrupted();
};
