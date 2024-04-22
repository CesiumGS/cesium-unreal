// Copyright 2020-2024 CesiumGS, Inc. and Contributors

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

  // Create curve
  std::optional<CesiumGeospatial::SimplePlanarEllipsoidCurve> curve =
      CesiumGeospatial::SimplePlanarEllipsoidCurve::
          fromEarthCenteredEarthFixedCoordinates(
              CesiumGeospatial::Ellipsoid::WGS84,
              glm::dvec3(ecefSource.X, ecefSource.Y, ecefSource.Z),
              glm::dvec3(
                  EarthCenteredEarthFixedDestination.X,
                  EarthCenteredEarthFixedDestination.Y,
                  EarthCenteredEarthFixedDestination.Z));

  if (!curve.has_value()) {
    return;
  }

  this->_currentCurve =
      MakeUnique<CesiumGeospatial::SimplePlanarEllipsoidCurve>(curve.value());

  this->_length = (EarthCenteredEarthFixedDestination - ecefSource).Length();

  // The source and destination rotations are expressed in East-South-Up
  // coordinates.
  this->_sourceRotation = this->GetCurrentRotationEastSouthUp();
  this->_destinationRotation =
      FRotator(PitchAtDestination, YawAtDestination, 0.0).Quaternion();

  this->_currentFlyTime = 0.0f;

  // Compute a wanted height from curve
  this->_maxHeight = 0.0;
  if (this->HeightPercentageCurve != NULL) {
    this->_maxHeight = 30000.0;
    if (this->MaximumHeightByDistanceCurve != NULL) {
      this->_maxHeight =
          this->MaximumHeightByDistanceCurve->GetFloatValue(this->_length);
    }
  }

  // Tell the tick we will be flying from now
  this->_canInterruptByMoving = CanInterruptByMoving;
  this->_previousPositionEcef = ecefSource;
  this->_flightInProgress = true;
  this->_destinationEcef = EarthCenteredEarthFixedDestination;
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

  check(this->_currentCurve);

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
      (this->_length == 0.0 &&
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

  // Get altitude offset from profile curve if one is specified
  double altitudeOffset = 0.0;
  if (this->_maxHeight != 0.0 && this->HeightPercentageCurve) {
    double curveOffset =
        this->_maxHeight *
        this->HeightPercentageCurve->GetFloatValue(flyPercentage);
    altitudeOffset = curveOffset;
  }

  glm::dvec3 currentPositionEcef =
      _currentCurve->getPosition(flyPercentage, altitudeOffset);

  FVector currentPositionVector(
      currentPositionEcef.x,
      currentPositionEcef.y,
      currentPositionEcef.z);

  // Set Location
  GlobeAnchor->MoveToEarthCenteredEarthFixedPosition(currentPositionVector);

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
