#include "CesiumFlyToComponent.h"
#include "CesiumGeoreference.h"
#include "CesiumGlobeAnchorComponent.h"
#include "CesiumWgs84Ellipsoid.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"
#include "VecMath.h"

#include <glm/gtx/quaternion.hpp>

UCesiumFlyToComponent::UCesiumFlyToComponent() {
  // Structure to hold one-time initialization
  struct FConstructorStatics {
    ConstructorHelpers::FObjectFinder<UCurveFloat> ProgressCurve;
    ConstructorHelpers::FObjectFinder<UCurveFloat> HeightPercentageCurve;
    ConstructorHelpers::FObjectFinder<UCurveFloat> MaximumHeightByDistanceCurve;
    FConstructorStatics()
        : ProgressCurve(TEXT(
              "/CesiumForUnreal/Curves/FlyTo/Curve_CesiumFlyToDefaultProgress_Float.Curve_CesiumFlyToDefaultProgress_Float")),
          HeightPercentageCurve(TEXT(
              "/CesiumForUnreal/Curves/FlyTo/Curve_CesiumFlyToDefaultHeightPercentage_Float.Curve_CesiumFlyToDefaultHeightPercentage_Float")),
          MaximumHeightByDistanceCurve(TEXT(
              "/CesiumForUnreal/Curves/FlyTo/Curve_CesiumFlyToDefaultMaximumHeightByDistance_Float.Curve_CesiumFlyToDefaultMaximumHeightByDistance_Float")) {
    }
  };
  static FConstructorStatics ConstructorStatics;

  this->ProgressCurve = ConstructorStatics.ProgressCurve.Object;
  this->HeightPercentageCurve = ConstructorStatics.HeightPercentageCurve.Object;
  this->MaximumHeightByDistanceCurve =
      ConstructorStatics.MaximumHeightByDistanceCurve.Object;

  this->PrimaryComponentTick.bCanEverTick = true;
}

void UCesiumFlyToComponent::FlyToLocationEarthCenteredEarthFixed(
    const FVector& EarthCenteredEarthFixedDestination,
    double YawAtDestination,
    double PitchAtDestination,
    bool CanInterruptByMoving) {
  if (this->_flightInProgress) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT("Cannot start a flight because one is already in progress."));
    return;
  }

  UCesiumGlobeAnchorComponent* GlobeAnchor = this->GetGlobeAnchor();
  if (!IsValid(GlobeAnchor)) {
    return;
  }

  PitchAtDestination = glm::clamp(PitchAtDestination, -89.99, 89.99);
  // Compute source location in ECEF
  FVector ecefSource = GlobeAnchor->GetEarthCenteredEarthFixedPosition();

  // The source and destination rotations are expressed in East-South-Up
  // coordinates.
  this->_sourceRotation = this->GetCurrentRotationEastSouthUp();
  this->_destinationRotation =
      FRotator(PitchAtDestination, YawAtDestination, 0.0).Quaternion();
  this->_destinationEcef = EarthCenteredEarthFixedDestination;

  // Compute axis/Angle transform
  glm::dvec3 glmEcefSource = VecMath::createVector3D(
      UCesiumWgs84Ellipsoid::ScaleToGeodeticSurface(ecefSource));
  glm::dvec3 glmEcefDestination = VecMath::createVector3D(
      UCesiumWgs84Ellipsoid::ScaleToGeodeticSurface(this->_destinationEcef));

  glm::dquat flyQuat = glm::rotation(
      glm::normalize(glmEcefSource),
      glm::normalize(glmEcefDestination));

  glm::dvec3 flyToRotationAxis = glm::axis(flyQuat);
  this->_rotationAxis.Set(
      flyToRotationAxis.x,
      flyToRotationAxis.y,
      flyToRotationAxis.z);

  this->_totalAngle = glm::angle(flyQuat);
  this->_totalAngle = CesiumUtility::Math::radiansToDegrees(this->_totalAngle);

  this->_currentFlyTime = 0.0f;

  // We will not create a curve projected along the ellipsoid as we want to take
  // altitude while flying. The radius of the current point will evolve as
  // follows:
  //  - Project the point on the ellipsoid - Will give a default radius
  //  depending on ellipsoid location.
  //  - Interpolate the altitudes : get source/destination altitude, and make a
  //  linear interpolation between them. This will allow for flying from/to any
  //  point smoothly.
  //  - Add as flightProfile offset /-\ defined by a curve.

  // Compute actual altitude at source and destination points by getting their
  // cartographic height
  this->_sourceHeight = 0.0;
  this->_destinationHeight = 0.0;

  FVector cartographicSource =
      UCesiumWgs84Ellipsoid::EarthCenteredEarthFixedToLongitudeLatitudeHeight(
          ecefSource);
  this->_sourceHeight = cartographicSource.Z;

  cartographicSource.Z = 0.0;
  FVector zeroHeightSource =
      UCesiumWgs84Ellipsoid::LongitudeLatitudeHeightToEarthCenteredEarthFixed(
          cartographicSource);

  this->_sourceDirection = zeroHeightSource.GetSafeNormal();

  FVector cartographicDestination =
      UCesiumWgs84Ellipsoid::EarthCenteredEarthFixedToLongitudeLatitudeHeight(
          EarthCenteredEarthFixedDestination);
  this->_destinationHeight = cartographicDestination.Z;

  // Compute a wanted height from curve
  this->_maxHeight = 0.0;
  if (this->HeightPercentageCurve != NULL) {
    this->_maxHeight = 30000.0;
    if (this->MaximumHeightByDistanceCurve != NULL) {
      double flyToDistance =
          (EarthCenteredEarthFixedDestination - ecefSource).Length();
      this->_maxHeight =
          this->MaximumHeightByDistanceCurve->GetFloatValue(flyToDistance);
    }
  }

  // Tell the tick we will be flying from now
  this->_canInterruptByMoving = CanInterruptByMoving;
  this->_previousPositionEcef = ecefSource;
  this->_flightInProgress = true;
}

void UCesiumFlyToComponent::FlyToLocationLongitudeLatitudeHeight(
    const FVector& LongitudeLatitudeHeightDestination,
    double YawAtDestination,
    double PitchAtDestination,
    bool CanInterruptByMoving) {
  FVector Ecef =
      UCesiumWgs84Ellipsoid::LongitudeLatitudeHeightToEarthCenteredEarthFixed(
          LongitudeLatitudeHeightDestination);
  this->FlyToLocationEarthCenteredEarthFixed(
      Ecef,
      YawAtDestination,
      PitchAtDestination,
      CanInterruptByMoving);
}

void UCesiumFlyToComponent::FlyToLocationUnreal(
    const FVector& UnrealDestination,
    double YawAtDestination,
    double PitchAtDestination,
    bool CanInterruptByMoving) {
  UCesiumGlobeAnchorComponent* GlobeAnchor = this->GetGlobeAnchor();
  if (!IsValid(GlobeAnchor)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "CesiumFlyToComponent cannot FlyToLocationUnreal because the Actor has no CesiumGlobeAnchorComponent."));
    return;
  }

  ACesiumGeoreference* Georeference = GlobeAnchor->ResolveGeoreference();
  if (!IsValid(Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "CesiumFlyToComponent cannot FlyToLocationUnreal because the globe anchor has no associated CesiumGeoreference."));
    return;
  }

  this->FlyToLocationEarthCenteredEarthFixed(
      Georeference->TransformUnrealPositionToEarthCenteredEarthFixed(
          UnrealDestination),
      YawAtDestination,
      PitchAtDestination,
      CanInterruptByMoving);
}

void UCesiumFlyToComponent::InterruptFlight() {
  this->_flightInProgress = false;

  UCesiumGlobeAnchorComponent* GlobeAnchor = this->GetGlobeAnchor();
  if (IsValid(GlobeAnchor)) {
    // fix Actor roll to 0.0
    FRotator currentRotator = this->GetCurrentRotationEastSouthUp().Rotator();
    currentRotator.Roll = 0.0;
    FQuat eastSouthUpRotation = currentRotator.Quaternion();
    this->SetCurrentRotationEastSouthUp(eastSouthUpRotation);
  }

  // Trigger callback accessible from BP
  UE_LOG(LogCesium, Verbose, TEXT("Broadcasting OnFlightInterrupt"));
  OnFlightInterrupted.Broadcast();
}

void UCesiumFlyToComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  if (!this->_flightInProgress) {
    return;
  }

  UCesiumGlobeAnchorComponent* GlobeAnchor = this->GetGlobeAnchor();
  if (!IsValid(GlobeAnchor)) {
    return;
  }

  if (this->_canInterruptByMoving &&
      this->_previousPositionEcef !=
          GlobeAnchor->GetEarthCenteredEarthFixedPosition()) {
    this->InterruptFlight();
    return;
  }

  this->_currentFlyTime += DeltaTime;

  // In order to accelerate at start and slow down at end, we use a progress
  // profile curve
  float flyPercentage;
  if (this->_currentFlyTime >= this->Duration) {
    flyPercentage = 1.0f;
  } else if (this->ProgressCurve) {
    flyPercentage = glm::clamp(
        this->ProgressCurve->GetFloatValue(
            this->_currentFlyTime / this->Duration),
        0.0f,
        1.0f);
  } else {
    flyPercentage = this->_currentFlyTime / this->Duration;
  }

  // If we reached the end, set actual destination location and
  // orientation
  if (flyPercentage >= 1.0f ||
      (this->_totalAngle == 0.0 &&
       this->_sourceRotation == this->_destinationRotation)) {
    GlobeAnchor->MoveToEarthCenteredEarthFixedPosition(this->_destinationEcef);
    this->SetCurrentRotationEastSouthUp(this->_destinationRotation);
    this->_flightInProgress = false;
    this->_currentFlyTime = 0.0f;

    // Trigger callback accessible from BP
    UE_LOG(LogCesium, Verbose, TEXT("Broadcasting OnFlightComplete"));
    OnFlightComplete.Broadcast();

    return;
  }

  // We're currently in flight. Interpolate the position and orientation:

  // Get the current position by interpolating with flyPercentage
  // Rotate our normalized source direction, interpolating with time
  FVector rotatedDirection = this->_sourceDirection.RotateAngleAxis(
      flyPercentage * this->_totalAngle,
      this->_rotationAxis);

  // Map the result to a position on our reference ellipsoid
  FVector geodeticPosition =
      UCesiumWgs84Ellipsoid::ScaleToGeodeticSurface(rotatedDirection);

  // Calculate the geodetic up at this position
  FVector geodeticUp =
      UCesiumWgs84Ellipsoid::GeodeticSurfaceNormal(geodeticPosition);

  // Add the altitude offset. Start with linear path between source and
  // destination If we have a profile curve, use this as well
  double altitudeOffset =
      glm::mix(this->_sourceHeight, this->_destinationHeight, flyPercentage);
  if (this->_maxHeight != 0.0 && this->HeightPercentageCurve) {
    double curveOffset =
        this->_maxHeight *
        this->HeightPercentageCurve->GetFloatValue(flyPercentage);
    altitudeOffset += curveOffset;
  }

  FVector currentPosition = geodeticPosition + geodeticUp * altitudeOffset;

  // Set Location
  GlobeAnchor->MoveToEarthCenteredEarthFixedPosition(currentPosition);

  // Interpolate rotation in the ESU frame. The local ESU ControlRotation will
  // be transformed to the appropriate world rotation as we fly.
  FQuat currentQuat = FQuat::Slerp(
      this->_sourceRotation,
      this->_destinationRotation,
      flyPercentage);
  this->SetCurrentRotationEastSouthUp(currentQuat);

  this->_previousPositionEcef =
      GlobeAnchor->GetEarthCenteredEarthFixedPosition();
}

FQuat UCesiumFlyToComponent::GetCurrentRotationEastSouthUp() {
  if (this->RotationToUse != ECesiumFlyToRotation::Actor) {
    APawn* Pawn = Cast<APawn>(this->GetOwner());
    const TObjectPtr<AController> Controller =
        IsValid(Pawn) ? Pawn->Controller : nullptr;
    if (Controller) {
      FRotator rotator = Controller->GetControlRotation();
      if (this->RotationToUse ==
          ECesiumFlyToRotation::ControlRotationInUnreal) {
        USceneComponent* PawnRoot = Pawn->GetRootComponent();
        if (IsValid(PawnRoot)) {
          rotator = GetGlobeAnchor()
                        ->ResolveGeoreference()
                        ->TransformUnrealRotatorToEastSouthUp(
                            Controller->GetControlRotation(),
                            PawnRoot->GetRelativeLocation());
        }
      }
      return rotator.Quaternion();
    }
  }

  return this->GetGlobeAnchor()->GetEastSouthUpRotation();
}

void UCesiumFlyToComponent::SetCurrentRotationEastSouthUp(
    const FQuat& EastSouthUpRotation) {
  bool controlRotationSet = false;

  if (this->RotationToUse != ECesiumFlyToRotation::Actor) {
    APawn* Pawn = Cast<APawn>(this->GetOwner());
    const TObjectPtr<AController> Controller =
        IsValid(Pawn) ? Pawn->Controller : nullptr;
    if (Controller) {
      FRotator rotator = EastSouthUpRotation.Rotator();
      if (this->RotationToUse ==
          ECesiumFlyToRotation::ControlRotationInUnreal) {
        USceneComponent* PawnRoot = Pawn->GetRootComponent();
        if (IsValid(PawnRoot)) {
          rotator = GetGlobeAnchor()
                        ->ResolveGeoreference()
                        ->TransformEastSouthUpRotatorToUnreal(
                            EastSouthUpRotation.Rotator(),
                            PawnRoot->GetRelativeLocation());
        }
      }

      Controller->SetControlRotation(EastSouthUpRotation.Rotator());
      controlRotationSet = true;
    }
  }

  if (!controlRotationSet) {
    this->GetGlobeAnchor()->SetEastSouthUpRotation(EastSouthUpRotation);
  }
}
