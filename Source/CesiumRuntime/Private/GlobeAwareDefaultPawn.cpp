// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "GlobeAwareDefaultPawn.h"
#include "Camera/CameraComponent.h"
#include "CesiumActors.h"
#include "CesiumGeoreference.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumGeospatial/Transforms.h"
#include "CesiumRuntime.h"
#include "CesiumTransforms.h"
#include "CesiumUtility/Math.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GlmLogging.h"
#include "VecMath.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>

AGlobeAwareDefaultPawn::AGlobeAwareDefaultPawn() : ADefaultPawn() {
  PrimaryActorTick.bCanEverTick = true;
}

void AGlobeAwareDefaultPawn::MoveRight(float Val) {

  // Note: This is overridden from ADefaultPawn to not
  // use ControlSpaceRot = Controller->GetControlRotation()
  // but ControlSpaceRot = this->GetViewRotation() instead!
  if (Val != 0.f) {
    if (Controller) {
      FRotator const ControlSpaceRot = this->GetViewRotation();

      // transform to world space and add it
      AddMovementInput(
          FRotationMatrix(ControlSpaceRot).GetScaledAxis(EAxis::Y),
          Val);

      if (this->_bFlyingToLocation && this->_bCanInterruptFlight) {
        this->_interruptFlight();
      }
    }
  }
}

void AGlobeAwareDefaultPawn::MoveForward(float Val) {

  // Note: This is overridden from ADefaultPawn to not
  // use ControlSpaceRot = Controller->GetControlRotation()
  // but ControlSpaceRot = this->GetViewRotation() instead!
  if (Val != 0.f) {
    if (Controller) {
      FRotator const ControlSpaceRot = this->GetViewRotation();

      // transform to world space and add it
      AddMovementInput(
          FRotationMatrix(ControlSpaceRot).GetScaledAxis(EAxis::X),
          Val);

      if (this->_bFlyingToLocation && this->_bCanInterruptFlight) {
        this->_interruptFlight();
      }
    }
  }
}

FVector
AGlobeAwareDefaultPawn::_computeEllipsoidNormalUnreal(const FVector& location) {

  if (!IsValid(this->Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("GlobeAwareDefaultPawn %s does not have a valid Georeference"),
        *this->GetName());
    return FVector();
  }
  glm::dvec3 ecef = this->Georeference->TransformUnrealToEcef(
      glm::dvec3(location.X, location.Y, location.Z));
  const glm::dvec3 ellipsoidNormalEcef =
      this->Georeference->ComputeGeodeticSurfaceNormal(ecef);
  const glm::dmat4& ecefToUnreal =
      this->Georeference->getGeoTransforms()
          .GetEllipsoidCenteredToUnrealWorldTransform();
  const glm::dvec3 ellipsoidNormalUnreal =
      glm::dvec3(ecefToUnreal * glm::dvec4(ellipsoidNormalEcef, 0.0));
  return VecMath::createVector(glm::normalize(ellipsoidNormalUnreal));
}

void AGlobeAwareDefaultPawn::MoveUp_World(float Val) {
  if (Val != 0.f) {
    FVector location = this->GetActorLocation();
    FVector up = _computeEllipsoidNormalUnreal(location);
    AddMovementInput(up, Val);

    if (this->_bFlyingToLocation && this->_bCanInterruptFlight) {
      this->_interruptFlight();
    }
  }
}

FRotator AGlobeAwareDefaultPawn::GetViewRotation() const {
  FRotator localRotation = ADefaultPawn::GetViewRotation();

  FMatrix enuAdjustmentMatrix =
      this->Georeference->InaccurateComputeEastNorthUpToUnreal(
          this->GetActorLocation());

  return FRotator(enuAdjustmentMatrix.ToQuat() * localRotation.Quaternion());
}

FRotator AGlobeAwareDefaultPawn::GetBaseAimRotation() const {
  return this->GetViewRotation();
}

glm::dvec3 AGlobeAwareDefaultPawn::GetECEFCameraLocation() const {
  FVector ueLocation = this->GetActorLocation();
  const glm::dvec3 ueLocationVec(ueLocation.X, ueLocation.Y, ueLocation.Z);
  if (!IsValid(this->Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("GlobeAwareDefaultPawn %s does not have a valid Georeference"),
        *this->GetName());
    return ueLocationVec;
  }
  glm::dvec3 ecef = this->Georeference->TransformUnrealToEcef(ueLocationVec);

  // TODO XXX This should be the same (maybe except for floating point errors),
  // except for the issue that the currentEcef is updated in Tick()...
  glm::dvec3 expectedEcef(currentEcefX, currentEcefY, currentEcefZ);
  if (ecef != expectedEcef) {
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("ecef is  %f %f %f in %s"),
        ecef.x,
        ecef.y,
        ecef.z,
        *this->GetName());
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("expected %f %f %f"),
        expectedEcef.x,
        expectedEcef.y,
        expectedEcef.z);
  }

  return ecef;
}

void AGlobeAwareDefaultPawn::SetECEFCameraLocation(const glm::dvec3& ecef) {
  glm::dvec3 ue;
  if (!IsValid(this->Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("GlobeAwareDefaultPawn %s does not have a valid Georeference"),
        *this->GetName());
    ue = ecef;
  } else {
    ue = this->Georeference->TransformEcefToUnreal(ecef);
  }
  this->currentEcefX = ecef.x;
  this->currentEcefY = ecef.y;
  this->currentEcefZ = ecef.z;
  ADefaultPawn::SetActorLocation(FVector(
      static_cast<float>(ue.x),
      static_cast<float>(ue.y),
      static_cast<float>(ue.z)));
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
  glm::dvec3 ECEFSource = this->GetECEFCameraLocation();

  // Compute the source and destination rotations in ENU
  // As during the flight, we can go around the globe, this is better to
  // interpolate in ENU coordinates
  this->_flyToSourceRotation = ADefaultPawn::GetViewRotation();
  this->_flyToDestinationRotation =
      FRotator(PitchAtDestination, YawAtDestination, 0);

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

  if (steps == 0) {
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
  double flyTodistance = glm::length(ECEFDestination - ECEFSource);

  // Add first keypoint
  this->_keypoints.push_back(ECEFSource);
  // DrawDebugPoint(GetWorld(),
  // this->Georeference->InaccurateTransformEcefToUe(ECEFSource), 8,
  // FColor::Red, true, 30);
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
              this->FlyToMaximumAltitudeCurve->GetFloatValue(flyTodistance));
        }
        offsetAltitude = static_cast<double>(
            maxAltitude *
            this->FlyToAltitudeProfileCurve->GetFloatValue(percentage));
      }

      glm::dvec3 point = *scaled + upVector * (altitude + offsetAltitude);
      this->_keypoints.push_back(point);
      // DrawDebugPoint(GetWorld(),
      // this->Georeference->InaccurateTransformEcefToUe(point), 8, FColor::Red,
      // true, 30);
    }
  }
  this->_keypoints.push_back(ECEFDestination);
  // DrawDebugPoint(GetWorld(),
  // this->Georeference->InaccurateTransformEcefToUe(ECEFDestination), 8,
  // FColor::Red, true, 30);

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

  if (!IsValid(this->Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("GlobeAwareDefaultPawn %s does not have a valid Georeference"),
        *this->GetName());
  }
  const glm::dvec3& ecef =
      this->Georeference->TransformLongitudeLatitudeHeightToEcef(
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

void AGlobeAwareDefaultPawn::HandleGeoreferenceUpdated() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called HandleGeoreferenceUpdated for %s"),
      *this->GetName());

  // TODO XXX DEBUG OUTPUT
  FVector ueLocation = this->GetActorLocation();
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT(
          "In HandleGeoreferenceUpdated %s geoRef is %s, ecef at %f %f %f, UE was at %f %f %f"),
      *this->GetName(),
      this->Georeference ? *FString("non-null") : *FString("null"),
      currentEcefX,
      currentEcefY,
      currentEcefZ,
      ueLocation.X,
      ueLocation.Y,
      ueLocation.Z);
  // TODO XXX DEBUG OUTPUT

  glm::dvec3 ue = this->Georeference->TransformEcefToUnreal(
      glm::dvec3(currentEcefX, currentEcefY, currentEcefZ));
  ADefaultPawn::SetActorLocation(FVector(
      static_cast<float>(ue.x),
      static_cast<float>(ue.y),
      static_cast<float>(ue.z)));

  // TODO XXX DEBUG OUTPUT
  ueLocation = this->GetActorLocation();
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT(
          "In HandleGeoreferenceUpdated %s geoRef is %s, ecef at %f %f %f, UE now at %f %f %f"),
      *this->GetName(),
      this->Georeference ? *FString("non-null") : *FString("null"),
      currentEcefX,
      currentEcefY,
      currentEcefZ,
      ueLocation.X,
      ueLocation.Y,
      ueLocation.Z);
  // TODO XXX DEBUG OUTPUT
}

bool AGlobeAwareDefaultPawn::ShouldTickIfViewportsOnly() const { return true; }

void AGlobeAwareDefaultPawn::_handleFlightStep(float DeltaSeconds) {

  if (!IsValid(this->Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("GlobeAwareDefaultPawn %s does not have a valid Georeference"),
        *this->GetName());
    return;
  }
  if (!this->GetWorld()->IsGameWorld() || !this->_bFlyingToLocation) {
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
    this->SetECEFCameraLocation(finalPoint);
    GetController()->SetControlRotation(this->_flyToDestinationRotation);
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

  // Get the current position by interpolating between those two points
  const glm::dvec3& lastPosition = this->_keypoints[lastIndex];
  const glm::dvec3& nextPosition = this->_keypoints[nextIndex];
  glm::dvec3 currentPosition =
      glm::mix(lastPosition, nextPosition, segmentPercentage);
  // Set Location
  this->SetECEFCameraLocation(currentPosition);

  // Interpolate rotation - Computation has to be done at each step because
  // the ENU CRS is depending on location. Do all calculations in double
  // precision until the very end.
  const glm::dquat startingQuat =
      Georeference->TransformRotatorUnrealToEastNorthUp(
          VecMath::createQuaternion(this->_flyToSourceRotation.Quaternion()),
          this->_keypoints[0]);
  const glm::dquat endingQuat =
      Georeference->TransformRotatorUnrealToEastNorthUp(
          VecMath::createQuaternion(
              this->_flyToDestinationRotation.Quaternion()),
          this->_keypoints.back());
  const glm::dquat& currentQuat =
      glm::slerp(startingQuat, endingQuat, flyPercentage);
  GetController()->SetControlRotation(
      VecMath::createRotator(Georeference->TransformRotatorEastNorthUpToUnreal(
          currentQuat,
          currentPosition)));
}

void AGlobeAwareDefaultPawn::Tick(float DeltaSeconds) {
  Super::Tick(DeltaSeconds);

  _handleFlightStep(DeltaSeconds);

  // track current ecef in case we need to restore it on georeference update

  // TODO The `currentEcef` should always contain the latest and correct values,
  // but it is not clear where all the "AddMovementInput" stuff is collapsed
  // into an actual update of the actor location....
  const glm::dvec3 newEcef = this->GetECEFCameraLocation();
  if (newEcef != glm::dvec3(currentEcefX, currentEcefY, currentEcefZ)) {
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("In tick of %s update ecef at %f %f %f to %f %f %f"),
        *this->GetName(),
        currentEcefX,
        currentEcefY,
        currentEcefZ,
        newEcef.x,
        newEcef.y,
        newEcef.z);
  }
  this->currentEcefX = newEcef.x;
  this->currentEcefY = newEcef.y;
  this->currentEcefZ = newEcef.z;
}

void AGlobeAwareDefaultPawn::BeginPlay() {
  Super::BeginPlay();
  // TODO: find more elegant solution
  // the controller gets confused if the pawn itself has a nonzero orientation
  // I get confused when I read comments like this one.
  this->SetActorRotation(FRotator(0.0, 0.0, 0.0));
}

void AGlobeAwareDefaultPawn::OnConstruction(const FTransform& Transform) {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called OnConstruction on actor %s"),
      *this->GetName());
  Super::OnConstruction(Transform);
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("In OnConstruction currentEcef is  %f %f %f in %s"),
        currentEcefX,
        currentEcefY,
        currentEcefZ,
        *this->GetName());
}

void AGlobeAwareDefaultPawn::PostActorCreated() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called PostActorCreated on actor %s"),
      *this->GetName());
  Super::PostActorCreated();
  _initGeoreference();
}
void AGlobeAwareDefaultPawn::PostLoad() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called PostLoad on actor %s"),
      *this->GetName());
  Super::PostLoad();
  _initGeoreference();
}
void AGlobeAwareDefaultPawn::_initGeoreference() {
  if (!this->Georeference) {
    this->Georeference = ACesiumGeoreference::GetDefaultGeoreference(this);
  }
  if (this->Georeference) {
    this->Georeference->OnGeoreferenceUpdated.AddUniqueDynamic(
        this,
        &AGlobeAwareDefaultPawn::HandleGeoreferenceUpdated);
  }
  FVector ueLocation = this->GetActorLocation();
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT(
          "In _initGeoreference ties %s geoRef is %s, ecef at %f %f %f, UE at %f %f %f"),
      *this->GetName(),
      this->Georeference ? *FString("non-null") : *FString("null"),
      currentEcefX,
      currentEcefY,
      currentEcefZ,
      ueLocation.X,
      ueLocation.Y,
      ueLocation.Z);
}

// TODO Remove this, no longer needed
void AGlobeAwareDefaultPawn::PostInitProperties() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called PostInitProperties on actor %s"),
      *this->GetName());
  Super::PostInitProperties();
}

void AGlobeAwareDefaultPawn::_interruptFlight() {
  this->_bFlyingToLocation = false;

  // fix camera roll to 0.0
  FRotator currentRotator = GetController()->GetControlRotation();
  currentRotator.Roll = 0.0;
  GetController()->SetControlRotation(currentRotator);
}

#if WITH_EDITOR
void AGlobeAwareDefaultPawn::PreEditChange(FProperty* PropertyThatWillChange) {
  Super::PreEditChange(PropertyThatWillChange);

  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called PreEditChange for %s"),
      *this->GetName());

  // If the Georeference is modified, detach the `HandleGeoreferenceUpdated`
  // callback from the current instance
  FName propertyName = PropertyThatWillChange->GetFName();
  if (propertyName ==
      GET_MEMBER_NAME_CHECKED(AGlobeAwareDefaultPawn, Georeference)) {
    if (IsValid(this->Georeference)) {
      this->Georeference->OnGeoreferenceUpdated.RemoveAll(this);
    }
    return;
  }
}
void AGlobeAwareDefaultPawn::PostEditChangeProperty(
    FPropertyChangedEvent& event) {
  Super::PostEditChangeProperty(event);

  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called PostEditChangeProperty for %s"),
      *this->GetName());

  if (!event.Property) {
    return;
  }

  FName propertyName = event.Property->GetFName();
  if (propertyName ==
      GET_MEMBER_NAME_CHECKED(AGlobeAwareDefaultPawn, Georeference)) {
    if (IsValid(this->Georeference)) {
      this->Georeference->OnGeoreferenceUpdated.AddUniqueDynamic(
          this,
          &AGlobeAwareDefaultPawn::HandleGeoreferenceUpdated);
    }
    return;
  }
}
#endif
