// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGeospatial/SimplePlanarEllipsoidCurve.h"
#include "CesiumGlobeAnchoredActorComponent.h"
#include "CesiumFlyToComponent.generated.h"

class UCurveFloat;
class UCesiumGlobeAnchorComponent;

/**
 * The delegate for when the Actor finishes flying.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCesiumFlightCompleted);

/**
 * The delegate for when the Actor's flight is interrupted.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCesiumFlightInterrupted);

/**
 * Indicates which rotation to use for orienting the object during flights.
 */
UENUM(BlueprintType)
enum class ECesiumFlyToRotation : uint8 {
  /**
   * Uses the relative rotation of the root component of the Actor to which the
   * CesiumFlyToComponent is attached.
   */
  Actor,

  /**
   * Uses the ControlRotation of the Controller of the Pawn to which the
   * CesiumFlyToComponent is attached. The ControlRotation is interpreted as
   * being relative to the Unreal coordinate system.
   *
   * If the component is attached to an Actor that is not a Pawn, or if the Pawn
   * does not have a Controller, this option is equivalent to the "Actor"
   * option.
   */
  ControlRotationInUnreal,

  /**
   * Uses the ControlRotation of the Controller of the Pawn to which the
   * CesiumFlyToComponent is attached. The ControlRotation is interpreted as
   * being relative to the Pawn's local East-South-Up coordinate system.
   *
   * This is the option to use with a GlobeAwareDefaultPawn or a DynamicPawn,
   * because those classes interpret the ControlRotation as being relative to
   * East-South-Up.
   *
   * If the component is attached to an Actor that is not a Pawn, or if the Pawn
   * does not have a Controller, this option is equivalent to the "Actor"
   * option.
   */
  ControlRotationInEastSouthUp
};

/**
 * Smoothly animates the Actor to which it is attached on a flight to a new
 * location on the globe.
 */
UCLASS(ClassGroup = "Cesium", Meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumFlyToComponent
    : public UCesiumGlobeAnchoredActorComponent {
  GENERATED_BODY()

public:
  UCesiumFlyToComponent();

  /**
   * A curve that is used to determine the flight progress percentage for all
   * the other curves. The input is the fraction (0.0 to 1.0) of the total time
   * that has passed so far, and the output is the fraction of the total curve
   * that should be traversed at this time. This curve allows the Actor to
   * accelerate and deaccelerate as desired throughout the flight.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  UCurveFloat* ProgressCurve;

  /**
   * A curve that controls what percentage of the maximum height the Actor
   * should take at a given time on the flight. This curve must be kept in the 0
   * to 1 range on both axes. The MaximumHeightByDistanceCurve controls the
   * actual maximum height that is achieved during the flight.
   *
   * If this curve is not specified, the height will be a smooth interpolation
   * between the height at the original location and the height at the
   * destination location, and the MaximumHeightByDistanceCurve will be ignored.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  UCurveFloat* HeightPercentageCurve;

  /**
   * A curve that controls the maximum height that will be achieved during the
   * flight as a function of the straight-line distance of the flight, in
   * meters. If the start and end point are on opposite sides of the globe, the
   * straight-line distance goes through the Earth even though the flight itself
   * will not.
   *
   * If HeightPercentageCurve is not specified, this property is ignored.
   * If HeightPercentageCurve is specified, but this property is not, then the
   * maximum height is 30,000 meters regardless of distance.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  UCurveFloat* MaximumHeightByDistanceCurve;

  /**
   * The length in seconds that the flight should last.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (ClampMin = 0.0))
  float Duration = 5.0f;

  /**
   * Indicates which rotation to use during flights.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  ECesiumFlyToRotation RotationToUse = ECesiumFlyToRotation::Actor;

  /**
   * A delegate that will be called when the Actor finishes flying.
   *
   */
  UPROPERTY(BlueprintAssignable, Category = "Cesium");
  FCesiumFlightCompleted OnFlightComplete;

  /**
   * A delegate that will be called when the Actor's flight is interrupted.
   *
   */
  UPROPERTY(BlueprintAssignable, Category = "Cesium");
  FCesiumFlightInterrupted OnFlightInterrupted;

  /**
   * Begin a smooth flight to the given Earth-Centered, Earth-Fixed
   * (ECEF) destination, such that the Actor ends at the specified yaw and
   * pitch. The yaw and pitch are expressed relative to an East-South-Up frame
   * at the destination. The characteristics of the flight can be configured
   * with the properties on this component.
   *
   * If CanInterruptByMoving is true and the Actor moves independent of this
   * component, then the flight in progress will be canceled.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void FlyToLocationEarthCenteredEarthFixed(
      const FVector& EarthCenteredEarthFixedDestination,
      double YawAtDestination,
      double PitchAtDestination,
      bool CanInterruptByMoving);

  /**
   * Begin a smooth camera flight to the given WGS84 longitude in degrees (x),
   * latitude in degrees (y), and height in meters (z) such that the camera
   * ends at the given yaw and pitch. The yaw and pitch are expressed relative
   * to an East-South-Up frame at the destination. The characteristics of the
   * flight can be configured with the properties on this component.
   *
   * Note that the height is measured in meters above the WGS84 ellipsoid, and
   * should not be confused with a height relative to mean sea level, which may
   * be tens of meters different depending on where you are on the globe.
   *
   * If CanInterruptByMoving is true and the Actor moves independent of this
   * component, then the flight in progress will be canceled.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void FlyToLocationLongitudeLatitudeHeight(
      const FVector& LongitudeLatitudeHeightDestination,
      double YawAtDestination,
      double PitchAtDestination,
      bool CanInterruptByMoving);

  /**
   * Begin a smooth flight to the given destination in Unreal coordinates, such
   * that the Actor ends at the specified yaw and pitch. The yaw and pitch are
   * expressed relative to an East-South-Up frame at the destination. The
   * characteristics of the flight can be configured with the properties on this
   * component.
   *
   * If CanInterruptByMoving is true and the Actor moves independent of this
   * component, then the flight in progress will be canceled.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void FlyToLocationUnreal(
      const FVector& UnrealDestination,
      double YawAtDestination,
      double PitchAtDestination,
      bool CanInterruptByMoving);

  /**
   * Interrupts the flight that is currently in progress, leaving the Actor
   * wherever it is currently.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void InterruptFlight();

  /**
   * Returns true if a flight is currently in progress, false otherwise.
   */
  bool IsFlightInProgress();

protected:
  virtual void TickComponent(
      float DeltaTime,
      ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;

private:
  FQuat GetCurrentRotationEastSouthUp();
  void SetCurrentRotationEastSouthUp(const FQuat& EastSouthUpRotation);

  bool _flightInProgress = false;
  bool _canInterruptByMoving;
  float _currentFlyTime;
  double _maxHeight;
  FVector _destinationEcef;
  FQuat _sourceRotation;
  FQuat _destinationRotation;
  FVector _previousPositionEcef;
  TUniquePtr<CesiumGeospatial::SimplePlanarEllipsoidCurve> _currentCurve;
  double _length;
};
