// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "GlobeAwareDefaultPawn.h"
#include "Camera/CameraComponent.h"
#include "CesiumActors.h"
#include "CesiumCustomVersion.h"
#include "CesiumGeoreference.h"
#include "CesiumGlobeAnchorComponent.h"
#include "CesiumRuntime.h"
#include "CesiumTransforms.h"
#include "CesiumUtility/Math.h"
#include "CesiumWgs84Ellipsoid.h"
#include "Curves/CurveFloat.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "VecMath.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>

AGlobeAwareDefaultPawn::AGlobeAwareDefaultPawn() : ADefaultPawn() {
  PrimaryActorTick.bCanEverTick = true;

#if WITH_EDITOR
  this->SetIsSpatiallyLoaded(false);
#endif
  this->GlobeAnchor =
      CreateDefaultSubobject<UCesiumGlobeAnchorComponent>(TEXT("GlobeAnchor"));
}

void AGlobeAwareDefaultPawn::MoveRight(float Val) {
  this->_moveAlongViewAxis(EAxis::Y, Val);
}

void AGlobeAwareDefaultPawn::MoveForward(float Val) {
  this->_moveAlongViewAxis(EAxis::X, Val);
}

void AGlobeAwareDefaultPawn::MoveUp_World(float Val) {
  if (Val == 0.0f) {
    return;
  }

  ACesiumGeoreference* pGeoreference = this->GetGeoreference();
  if (!IsValid(pGeoreference)) {
    return;
  }

  FVector upEcef = UCesiumWgs84Ellipsoid::GeodeticSurfaceNormal(
      this->GlobeAnchor->GetECEF());
  FVector up =
      pGeoreference->TransformEarthCenteredEarthFixedDirectionToUnreal(upEcef);

  FTransform transform{};
  USceneComponent* pRootComponent = this->GetRootComponent();
  if (IsValid(pRootComponent)) {
    USceneComponent* pParent = pRootComponent->GetAttachParent();
    if (IsValid(pParent)) {
      transform = pParent->GetComponentToWorld();
    }
  }

  this->_moveAlongVector(transform.TransformVector(up), Val);
}

FRotator AGlobeAwareDefaultPawn::GetViewRotation() const {
  if (!Controller) {
    return this->GetActorRotation();
  }

  ACesiumGeoreference* pGeoreference = this->GetGeoreference();
  if (!pGeoreference) {
    return this->GetActorRotation();
  }

  // The control rotation is expressed in a left-handed East-South-Up (ESU)
  // coordinate system:
  // * Yaw: Clockwise from East: 0 is East, 90 degrees is
  // South, 180 degrees is West, 270 degrees is North.
  // * Pitch: Angle above level, Positive is looking up, negative is looking
  // down
  // * Roll: Rotation around the look direction. Positive is a barrel roll to
  // the right (clockwise).
  FRotator localRotation = Controller->GetControlRotation();

  FTransform transform{};
  USceneComponent* pRootComponent = this->GetRootComponent();
  if (IsValid(pRootComponent)) {
    USceneComponent* pParent = pRootComponent->GetAttachParent();
    if (IsValid(pParent)) {
      transform = pParent->GetComponentToWorld();
    }
  }

  // Transform the rotation in the ESU frame to the Unreal world frame.
  FVector globePosition =
      transform.InverseTransformPosition(this->GetPawnViewLocation());
  FMatrix esuAdjustmentMatrix =
      pGeoreference->ComputeEastSouthUpToUnrealTransformation(globePosition) *
      transform.ToMatrixNoScale();

  return FRotator(esuAdjustmentMatrix.ToQuat() * localRotation.Quaternion());
}

FRotator AGlobeAwareDefaultPawn::GetBaseAimRotation() const {
  return this->GetViewRotation();
}

void AGlobeAwareDefaultPawn::_interpolateFlightPosition(
    float percentage,
    glm::dvec3& out) const {

  // Rotate our normalized source direction, interpolating with time
  glm::dvec3 rotatedDirection = glm::rotate(
      _flyToSourceDirection,
      percentage * _flyToTotalAngle,
      _flyToRotationAxis);

  // Map the result to a position on our reference ellipsoid
  if (auto geodeticPosition =
          this->_ellipsoid.scaleToGeodeticSurface(rotatedDirection)) {

    // Calculate the geodetic up at this position
    glm::dvec3 geodeticUp =
        this->_ellipsoid.geodeticSurfaceNormal(*geodeticPosition);

    // Add the altitude offset. Start with linear path between source and
    // destination If we have a profile curve, use this as well
    double altitudeOffset =
        glm::mix(_flyToSourceAltitude, _flyToDestinationAltitude, percentage);
    if (_flyToMaxAltitude != 0.0 && this->FlyToAltitudeProfileCurve) {
      double curveOffset =
          _flyToMaxAltitude *
          this->FlyToAltitudeProfileCurve->GetFloatValue(percentage);
      altitudeOffset += curveOffset;
    }

    out = *geodeticPosition + geodeticUp * altitudeOffset;
  }
}

const FTransform&
AGlobeAwareDefaultPawn::GetGlobeToUnrealWorldTransform() const {
  AActor* pParent = this->GetAttachParentActor();
  if (IsValid(pParent)) {
    return pParent->GetActorTransform();
  }
  return FTransform::Identity;
}

void AGlobeAwareDefaultPawn::FlyToLocationECEF(
    const glm::dvec3& ECEFDestination,
    double YawAtDestination,
    double PitchAtDestination,
    bool CanInterruptByMoving) {

  if (this->_bFlyingToLocation) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT("Cannot start a flight because one is already in progress."));
    return;
  }

  if (!IsValid(this->Controller)) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Cannot start a flight because the pawn does not have a Controller. You probably need to \"possess\" it before attempting to initiate a flight."));
    return;
  }

  if (!IsValid(this->GetGeoreference())) {
    return;
  }

  PitchAtDestination = glm::clamp(PitchAtDestination, -89.99, 89.99);
  // Compute source location in ECEF
  glm::dvec3 ECEFSource = VecMath::createVector3D(this->GlobeAnchor->GetECEF());

  // The source and destination rotations are expressed in East-South-Up
  // coordinates.
  this->_flyToSourceRotation = Controller->GetControlRotation().Quaternion();
  this->_flyToDestinationRotation =
      FRotator(PitchAtDestination, YawAtDestination, 0).Quaternion();
  this->_flyToECEFDestination = ECEFDestination;

  // Compute axis/Angle transform
  glm::dquat flyQuat = glm::rotation(
      glm::normalize(ECEFSource),
      glm::normalize(ECEFDestination));
  _flyToTotalAngle = glm::angle(flyQuat);
  _flyToRotationAxis = glm::axis(flyQuat);

  this->_currentFlyTime = 0.0;

  if (_flyToTotalAngle == 0.0 &&
      this->_flyToSourceRotation == this->_flyToDestinationRotation) {
    return;
  }

  // We will not create a curve projected along the ellipsoid as we want to take
  // altitude while flying. The radius of the current point will evolve as
  // follow
  //  - Project the point on the ellipsoid - Will give a default radius
  //  depending on ellipsoid location.
  //  - Interpolate the altitudes : get source/destination altitude, and make a
  //  linear interpolation between them. This will allow for flying from/to any
  //  point smoothly.
  //  - Add as flightProfile offset /-\ defined by a curve.

  // Compute actual altitude at source and destination points by getting their
  // cartographic height
  _flyToSourceAltitude = 0.0;
  _flyToDestinationAltitude = 0.0;

  if (auto cartographicSource =
          this->_ellipsoid.cartesianToCartographic(ECEFSource)) {
    _flyToSourceAltitude = cartographicSource->height;

    cartographicSource->height = 0;
    glm::dvec3 zeroHeightSource =
        this->_ellipsoid.cartographicToCartesian(*cartographicSource);

    _flyToSourceDirection = glm::normalize(zeroHeightSource);
  }
  if (auto cartographic =
          this->_ellipsoid.cartesianToCartographic(ECEFDestination)) {
    _flyToDestinationAltitude = cartographic->height;
  }

  // Compute a wanted altitude from curve
  _flyToMaxAltitude = 0.0;
  if (this->FlyToAltitudeProfileCurve != NULL) {
    _flyToMaxAltitude = 30000;
    if (this->FlyToMaximumAltitudeCurve != NULL) {
      double flyToDistance = glm::length(ECEFDestination - ECEFSource);
      _flyToMaxAltitude =
          this->FlyToMaximumAltitudeCurve->GetFloatValue(flyToDistance);
    }
  }

  // Tell the tick we will be flying from now
  this->_bFlyingToLocation = true;
  this->_bCanInterruptFlight = CanInterruptByMoving;
}

void AGlobeAwareDefaultPawn::FlyToLocationECEF(
    const FVector& ECEFDestination,
    double YawAtDestination,
    double PitchAtDestination,
    bool CanInterruptByMoving) {

  this->FlyToLocationECEF(
      glm::dvec3(ECEFDestination.X, ECEFDestination.Y, ECEFDestination.Z),
      YawAtDestination,
      PitchAtDestination,
      CanInterruptByMoving);
}

void AGlobeAwareDefaultPawn::FlyToLocationLongitudeLatitudeHeight(
    const glm::dvec3& LongitudeLatitudeHeightDestination,
    double YawAtDestination,
    double PitchAtDestination,
    bool CanInterruptByMoving) {

  FVector ecef =
      UCesiumWgs84Ellipsoid::LongitudeLatitudeHeightToEarthCenteredEarthFixed(
          VecMath::createVector(LongitudeLatitudeHeightDestination));
  this->FlyToLocationECEF(
      ecef,
      YawAtDestination,
      PitchAtDestination,
      CanInterruptByMoving);
}

UFUNCTION(BlueprintCallable)
void AGlobeAwareDefaultPawn::FlyToLocationLongitudeLatitudeHeight(
    const FVector& LongitudeLatitudeHeightDestination,
    double YawAtDestination,
    double PitchAtDestination,
    bool CanInterruptByMoving) {

  this->FlyToLocationLongitudeLatitudeHeight(
      VecMath::createVector3D(LongitudeLatitudeHeightDestination),
      YawAtDestination,
      PitchAtDestination,
      CanInterruptByMoving);
}

bool AGlobeAwareDefaultPawn::ShouldTickIfViewportsOnly() const { return true; }

void AGlobeAwareDefaultPawn::_handleFlightStep(float DeltaSeconds) {
  if (!IsValid(this->GetGeoreference())) {
    return;
  }

  if (!this->GetWorld()->IsGameWorld() || !this->_bFlyingToLocation) {
    return;
  }

  if (!Controller) {
    return;
  }

  this->_currentFlyTime += DeltaSeconds;

  // In order to accelerate at start and slow down at end, we use a progress
  // profile curve
  float flyPercentage;
  if (this->_currentFlyTime >= this->FlyToDuration) {
    flyPercentage = 1.0f;
  } else if (this->FlyToProgressCurve) {
    flyPercentage = glm::clamp(
        this->FlyToProgressCurve->GetFloatValue(
            this->_currentFlyTime / this->FlyToDuration),
        0.0f,
        1.0f);
  } else {
    flyPercentage = this->_currentFlyTime / this->FlyToDuration;
  }

  // If we reached the end, set actual destination location and
  // orientation
  if (flyPercentage >= 1.0f) {
    this->GlobeAnchor->MoveToECEF(this->_flyToECEFDestination);
    Controller->SetControlRotation(this->_flyToDestinationRotation.Rotator());
    this->_bFlyingToLocation = false;
    this->_currentFlyTime = 0.0f;

    // Trigger callback accessible from BP
    UE_LOG(LogCesium, Verbose, TEXT("Broadcasting OnFlightComplete"));
    OnFlightComplete.Broadcast();

    return;
  }

  // We're currently in flight. Interpolate the position and orientation:

  // Get the current position by interpolating with flyPercentage
  glm::dvec3 currentPosition;
  _interpolateFlightPosition(flyPercentage, currentPosition);

  // Set Location
  this->GlobeAnchor->MoveToECEF(currentPosition);

  // Interpolate rotation in the ESU frame. The local ESU ControlRotation will
  // be transformed to the appropriate world rotation as we fly.
  FQuat currentQuat = FQuat::Slerp(
      this->_flyToSourceRotation,
      this->_flyToDestinationRotation,
      flyPercentage);
  Controller->SetControlRotation(currentQuat.Rotator());
}

void AGlobeAwareDefaultPawn::Tick(float DeltaSeconds) {
  Super::Tick(DeltaSeconds);

  _handleFlightStep(DeltaSeconds);
}

void AGlobeAwareDefaultPawn::PostLoad() {
  Super::PostLoad();

  // For backward compatibility, copy the value of the deprecated Georeference
  // property to its new home in the GlobeAnchor. It doesn't appear to be
  // possible to do this in Serialize:
  // https://udn.unrealengine.com/s/question/0D54z00007CAbHFCA1/backward-compatibile-serialization-for-uobject-pointers
  const int32 CesiumVersion =
      this->GetLinkerCustomVersion(FCesiumCustomVersion::GUID);
  if (CesiumVersion < FCesiumCustomVersion::GeoreferenceRefactoring) {
    if (this->Georeference_DEPRECATED != nullptr && this->GlobeAnchor &&
        this->GlobeAnchor->GetGeoreference() == nullptr) {
      this->GlobeAnchor->SetGeoreference(this->Georeference_DEPRECATED);
    }
  }
}

ACesiumGeoreference* AGlobeAwareDefaultPawn::GetGeoreference() const {
  if (!IsValid(this->GlobeAnchor)) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "GlobeAwareDefaultPawn %s does not have a valid GlobeAnchorComponent."),
        *this->GetName());
    return nullptr;
  }

  ACesiumGeoreference* pGeoreference = this->GlobeAnchor->ResolveGeoreference();
  if (!IsValid(pGeoreference)) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "GlobeAwareDefaultPawn %s does not have a valie CesiumGeoreference."),
        *this->GetName());
    pGeoreference = nullptr;
  }

  return pGeoreference;
}

void AGlobeAwareDefaultPawn::_moveAlongViewAxis(EAxis::Type axis, double Val) {
  if (Val == 0.0) {
    return;
  }

  FRotator worldRotation = this->GetViewRotation();
  this->_moveAlongVector(
      FRotationMatrix(worldRotation).GetScaledAxis(axis),
      Val);
}

void AGlobeAwareDefaultPawn::_moveAlongVector(
    const FVector& vector,
    double Val) {
  if (Val == 0.0) {
    return;
  }

  FRotator worldRotation = this->GetViewRotation();
  AddMovementInput(vector, Val);

  if (this->_bFlyingToLocation && this->_bCanInterruptFlight) {
    this->_interruptFlight();
  }
}

void AGlobeAwareDefaultPawn::_interruptFlight() {
  if (!Controller) {
    return;
  }

  this->_bFlyingToLocation = false;

  // fix camera roll to 0.0
  FRotator currentRotator = Controller->GetControlRotation();
  currentRotator.Roll = 0.0;
  Controller->SetControlRotation(currentRotator);

  // Trigger callback accessible from BP
  UE_LOG(LogCesium, Verbose, TEXT("Broadcasting OnFlightInterrupt"));
  OnFlightInterrupt.Broadcast();
}
