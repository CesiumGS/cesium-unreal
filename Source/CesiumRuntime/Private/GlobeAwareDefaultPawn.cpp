// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "GlobeAwareDefaultPawn.h"
#include "Camera/CameraComponent.h"
#include "CesiumActors.h"
#include "CesiumCustomVersion.h"
#include "CesiumGeoreference.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumGeospatial/Transforms.h"
#include "CesiumGlobeAnchorComponent.h"
#include "CesiumRuntime.h"
#include "CesiumTransforms.h"
#include "CesiumUtility/Math.h"
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
  if (Val == 0.0f || !IsValid(this->GlobeAnchor)) {
    return;
  }

  glm::dvec4 upEcef(
      CesiumGeospatial::Ellipsoid::WGS84.geodeticSurfaceNormal(
          this->GlobeAnchor->GetECEF()),
      0.0);
  glm::dvec4 up = this->GlobeAnchor->ResolveGeoreference()
                      ->GetGeoTransforms()
                      .GetEllipsoidCenteredToAbsoluteUnrealWorldTransform() *
                  upEcef;

  this->_moveAlongVector(FVector(up.x, up.y, up.z), Val);
}

FRotator AGlobeAwareDefaultPawn::GetViewRotation() const {
  if (!Controller) {
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

  // Transform the rotation in the ESU frame to the Unreal world frame.
  FMatrix enuAdjustmentMatrix =
      this->GetGeoreference()->InaccurateComputeEastNorthUpToUnreal(
          this->GetPawnViewLocation());

  return FRotator(enuAdjustmentMatrix.ToQuat() * localRotation.Quaternion());
}

FRotator AGlobeAwareDefaultPawn::GetBaseAimRotation() const {
  return this->GetViewRotation();
}

void AGlobeAwareDefaultPawn::FlyToLocationECEF(
    const glm::dvec3& ECEFDestination,
    float YawAtDestination,
    float PitchAtDestination,
    bool CanInterruptByMoving) {

  if (this->_bFlyingToLocation) {
    return;
  }

  // Compute source location in ECEF
  glm::dvec3 ECEFSource = this->GlobeAnchor->GetECEF();

  // The source and destination rotations are expressed in East-South-Up
  // coordinates.
  this->_flyToSourceRotation = Controller->GetControlRotation().Quaternion();
  this->_flyToDestinationRotation =
      FRotator(PitchAtDestination, YawAtDestination, 0).Quaternion();

  // Compute axis/Angle transform and initialize key points
  glm::dquat flyQuat = glm::rotation(
      glm::normalize(ECEFSource),
      glm::normalize(ECEFDestination));
  double flyTotalAngle = glm::angle(flyQuat);
  glm::dvec3 flyRotationAxis = glm::axis(flyQuat);
  int steps = glm::max(
      int(flyTotalAngle / glm::radians(this->FlyToGranularityDegrees)) - 1,
      0);
  this->_keypoints.clear();
  this->_currentFlyTime = 0.0;

  if (flyTotalAngle == 0.0 &&
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

  // Compute global radius at source and destination points
  double sourceRadius = glm::length(ECEFSource);
  glm::dvec3 sourceUpVector = ECEFSource;

  // Compute actual altitude at source and destination points by scaling on
  // ellipsoid.
  double sourceAltitude = 0.0, destinationAltitude = 0.0;
  const CesiumGeospatial::Ellipsoid& ellipsoid =
      CesiumGeospatial::Ellipsoid::WGS84;
  if (auto scaled = ellipsoid.scaleToGeodeticSurface(ECEFSource)) {
    sourceAltitude = glm::length(ECEFSource - *scaled);
  }
  if (auto scaled = ellipsoid.scaleToGeodeticSurface(ECEFDestination)) {
    destinationAltitude = glm::length(ECEFDestination - *scaled);
  }

  // Get distance between source and destination points to compute a wanted
  // altitude from curve
  double flyToDistance = glm::length(ECEFDestination - ECEFSource);

  // Add first keypoint
  this->_keypoints.push_back(ECEFSource);

  for (int step = 1; step <= steps; step++) {
    double percentage = (double)step / (steps + 1);
    double altitude = glm::mix(sourceAltitude, destinationAltitude, percentage);
    double phi =
        glm::radians(this->FlyToGranularityDegrees * static_cast<double>(step));

    glm::dvec3 rotated = glm::rotate(sourceUpVector, phi, flyRotationAxis);
    if (auto scaled = ellipsoid.scaleToGeodeticSurface(rotated)) {
      glm::dvec3 upVector = glm::normalize(*scaled);

      // Add an altitude if we have a profile curve for it
      double offsetAltitude = 0;
      if (this->FlyToAltitudeProfileCurve != NULL) {
        double maxAltitude = 30000;
        if (this->FlyToMaximumAltitudeCurve != NULL) {
          maxAltitude = static_cast<double>(
              this->FlyToMaximumAltitudeCurve->GetFloatValue(flyToDistance));
        }
        offsetAltitude = static_cast<double>(
            maxAltitude *
            this->FlyToAltitudeProfileCurve->GetFloatValue(percentage));
      }

      glm::dvec3 point = *scaled + upVector * (altitude + offsetAltitude);
      this->_keypoints.push_back(point);
    }
  }

  this->_keypoints.push_back(ECEFDestination);

  // Tell the tick we will be flying from now
  this->_bFlyingToLocation = true;
  this->_bCanInterruptFlight = CanInterruptByMoving;
}

void AGlobeAwareDefaultPawn::InaccurateFlyToLocationECEF(
    const FVector& ECEFDestination,
    float YawAtDestination,
    float PitchAtDestination,
    bool CanInterruptByMoving) {

  this->FlyToLocationECEF(
      glm::dvec3(ECEFDestination.X, ECEFDestination.Y, ECEFDestination.Z),
      YawAtDestination,
      PitchAtDestination,
      CanInterruptByMoving);
}

void AGlobeAwareDefaultPawn::FlyToLocationLongitudeLatitudeHeight(
    const glm::dvec3& LongitudeLatitudeHeightDestination,
    float YawAtDestination,
    float PitchAtDestination,
    bool CanInterruptByMoving) {

  if (!IsValid(this->GetGeoreference())) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("GlobeAwareDefaultPawn %s does not have a valid Georeference"),
        *this->GetName());
  }
  const glm::dvec3& ecef =
      this->GetGeoreference()->TransformLongitudeLatitudeHeightToEcef(
          LongitudeLatitudeHeightDestination);
  this->FlyToLocationECEF(
      ecef,
      YawAtDestination,
      PitchAtDestination,
      CanInterruptByMoving);
}

UFUNCTION(BlueprintCallable)
void AGlobeAwareDefaultPawn::InaccurateFlyToLocationLongitudeLatitudeHeight(
    const FVector& LongitudeLatitudeHeightDestination,
    float YawAtDestination,
    float PitchAtDestination,
    bool CanInterruptByMoving) {

  this->FlyToLocationLongitudeLatitudeHeight(
      VecMath::createVector3D(LongitudeLatitudeHeightDestination),
      YawAtDestination,
      PitchAtDestination,
      CanInterruptByMoving);
}

bool AGlobeAwareDefaultPawn::ShouldTickIfViewportsOnly() const { return true; }

void AGlobeAwareDefaultPawn::_handleFlightStep(float DeltaSeconds) {
  if (!IsValid(this->GlobeAnchor)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "GlobeAwareDefaultPawn %s does not have a valid GeoreferenceComponent"),
        *this->GetName());
    return;
  }

  if (!this->GetWorld()->IsGameWorld() || !this->_bFlyingToLocation) {
    return;
  }

  if (!Controller) {
    return;
  }

  this->_currentFlyTime += static_cast<double>(DeltaSeconds);

  // double check that we don't have an empty list of keypoints
  if (this->_keypoints.size() == 0) {
    this->_bFlyingToLocation = false;
    return;
  }

  // If we reached the end, set actual destination location and orientation
  if (this->_currentFlyTime >= this->FlyToDuration) {
    const glm::dvec3& finalPoint = _keypoints.back();
    this->GlobeAnchor->MoveToECEF(finalPoint);
    Controller->SetControlRotation(this->_flyToDestinationRotation.Rotator());
    this->_bFlyingToLocation = false;
    this->_currentFlyTime = 0.0;
    return;
  }

  // We're currently in flight. Interpolate the position and orientation:

  double rawPercentage = this->_currentFlyTime / this->FlyToDuration;

  // In order to accelerate at start and slow down at end, we use a progress
  // profile curve
  double flyPercentage = rawPercentage;
  if (this->FlyToProgressCurve != NULL) {
    flyPercentage = glm::clamp(
        static_cast<double>(
            this->FlyToProgressCurve->GetFloatValue(rawPercentage)),
        0.0,
        1.0);
  }

  // Find the keypoint indexes corresponding to the current percentage
  int lastIndex = glm::floor(flyPercentage * (this->_keypoints.size() - 1));
  double segmentPercentage =
      flyPercentage * (this->_keypoints.size() - 1) - lastIndex;
  int nextIndex = lastIndex + 1;

  // Get the current position by interpolating linearly between those two points
  const glm::dvec3& lastPosition = this->_keypoints[lastIndex];
  const glm::dvec3& nextPosition = this->_keypoints[nextIndex];
  glm::dvec3 currentPosition =
      glm::mix(lastPosition, nextPosition, segmentPercentage);
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
        TEXT("GlobeAwareDefaultPawn %s does not have a GlobeAnchorComponent"),
        *this->GetName());
    return nullptr;
  }
  return this->GlobeAnchor->ResolveGeoreference();
}

void AGlobeAwareDefaultPawn::_moveAlongViewAxis(EAxis::Type axis, float Val) {
  if (Val == 0.0f) {
    return;
  }

  FRotator worldRotation = this->GetViewRotation();
  this->_moveAlongVector(
      FRotationMatrix(worldRotation).GetScaledAxis(axis),
      Val);
}

void AGlobeAwareDefaultPawn::_moveAlongVector(
    const FVector& vector,
    float Val) {
  if (Val == 0.0f) {
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
}
