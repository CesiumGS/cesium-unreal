// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGeoreferenceComponent.h"
#include "CesiumGeospatial/Cartographic.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumGeospatial/Transforms.h"
#include "CesiumRuntime.h"
#include "CesiumTransforms.h"
#include "CesiumUtility/Math.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GlmLogging.h"
#include "UObject/NameTypes.h"
#include "VecMath.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <optional>

UCesiumGeoreferenceComponent::UCesiumGeoreferenceComponent()
    : _updatingActorTransform(false), _currentEcef(), _currentUnrealToEcef() {
  this->bAutoActivate = true;
  PrimaryComponentTick.bCanEverTick = false;
}

void UCesiumGeoreferenceComponent::SnapLocalUpToEllipsoidNormal() {

  if (!IsValid(this->Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "CesiumGeoreferenceComponent %s does not have a valid Georeference"),
        *this->GetName());
    return;
  }

  // If our ECEF position isn't valid, we need to compute it from the Actor's
  // Transform first.
  if (!this->_ecefIsValid) {
    this->_updateFromActor();
  }

  // Compute the local up axis of the actor (the +Z axis)
  const glm::dmat3 currentActorRotation = _getRotationFromActor();
  const glm::dvec3 actorUpUnreal = glm::normalize(currentActorRotation[2]);

  // Compute the surface normal of the ellipsoid
  const glm::dvec3 ellipsoidNormalUnreal =
      _computeEllipsoidNormalUnreal(_currentEcef);

  // The shortest rotation to align local up with the ellipsoid normal
  const glm::dquat R = glm::rotation(actorUpUnreal, ellipsoidNormalUnreal);
  const glm::dmat3 alignmentRotation = glm::mat3_cast(R);

  // Compute the new actor rotation
  const glm::dmat3 newActorRotation = alignmentRotation * currentActorRotation;
  const glm::dvec3 relativeLocation = _computeRelativeLocation(_currentEcef);

  this->_updateActorTransform(newActorRotation, relativeLocation);
}

void UCesiumGeoreferenceComponent::SnapToEastSouthUp() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called SnapToEastSouthUp on component %s"),
      *this->GetName());

  if (!IsValid(this->Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "CesiumGeoreferenceComponent %s does not have a valid Georeference"),
        *this->GetName());
    return;
  }

  const UWorld* world = this->GetWorld();
  if (!IsValid(world)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreferenceComponent %s is not spawned in world"),
        *this->GetName());
    return;
  }

  // If our ECEF position isn't valid, we need to compute it from the Actor's
  // Transform first.
  if (!this->_ecefIsValid) {
    this->_updateFromActor();
  }

  const glm::dmat3 newActorRotation =
      this->Georeference->ComputeEastNorthUpToUnreal(_currentEcef);
  const glm::dvec3 relativeLocation = _computeRelativeLocation(_currentEcef);

  this->_updateActorTransform(newActorRotation, relativeLocation);
}

void UCesiumGeoreferenceComponent::MoveToLongitudeLatitudeHeight(
    const glm::dvec3& targetLongitudeLatitudeHeight,
    bool maintainRelativeOrientation) {

  if (!IsValid(this->Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "CesiumGeoreferenceComponent %s does not have a valid Georeference"),
        *this->GetName());
    return;
  }
  glm::dvec3 ecef = this->Georeference->TransformLongitudeLatitudeHeightToEcef(
      targetLongitudeLatitudeHeight);

  this->_setECEF(ecef, maintainRelativeOrientation);
}

void UCesiumGeoreferenceComponent::InaccurateMoveToLongitudeLatitudeHeight(
    const FVector& targetLongitudeLatitudeHeight,
    bool maintainRelativeOrientation) {
  this->MoveToLongitudeLatitudeHeight(
      VecMath::createVector3D(targetLongitudeLatitudeHeight),
      maintainRelativeOrientation);
}

FVector
UCesiumGeoreferenceComponent::InaccurateGetLongitudeLatitudeHeight() const {
  return FVector(
      static_cast<float>(this->Longitude),
      static_cast<float>(this->Latitude),
      static_cast<float>(this->Height));
}

void UCesiumGeoreferenceComponent::MoveToECEF(
    const glm::dvec3& targetEcef,
    bool maintainRelativeOrientation) {
  this->_setECEF(targetEcef, maintainRelativeOrientation);
}

void UCesiumGeoreferenceComponent::InaccurateMoveToECEF(
    const FVector& targetEcef,
    bool maintainRelativeOrientation) {
  this->MoveToECEF(
      VecMath::createVector3D(targetEcef),
      maintainRelativeOrientation);
}

FVector UCesiumGeoreferenceComponent::InaccurateGetECEF() const {
  return FVector(
      static_cast<float>(this->ECEF_X),
      static_cast<float>(this->ECEF_Y),
      static_cast<float>(this->ECEF_Z));
}

void UCesiumGeoreferenceComponent::OnRegister() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called OnRegister on component %s"),
      *this->GetName());
  Super::OnRegister();

  const AActor* const owner = this->GetOwner();
  if (!IsValid(owner)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreferenceComponent %s does not have a valid owner"),
        *this->GetName());
    return;
  }
  USceneComponent* ownerRoot = owner->GetRootComponent();
  if (ownerRoot) {
    ownerRoot->TransformUpdated.AddUObject(
        this,
        &UCesiumGeoreferenceComponent::HandleActorTransformUpdated);
  }

  this->_initGeoreference();

  this->_currentUnrealToEcef =
      this->Georeference->GetGeoTransforms()
          .GetAbsoluteUnrealWorldToEllipsoidCenteredTransform();

  if (!this->_ecefIsValid) {
    this->_updateFromActor();
  }
}

void UCesiumGeoreferenceComponent::OnUnregister() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called OnUnregister on component %s"),
      *this->GetName());
  Super::OnUnregister();

  const AActor* const owner = this->GetOwner();
  if (!IsValid(owner)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreferenceComponent %s does not have a valid owner"),
        *this->GetName());
    return;
  }

  if (this->Georeference) {
    this->Georeference->OnGeoreferenceUpdated.RemoveAll(this);
  }

  USceneComponent* ownerRoot = owner->GetRootComponent();
  if (ownerRoot) {
    ownerRoot->TransformUpdated.RemoveAll(this);
  }
}

void UCesiumGeoreferenceComponent::HandleActorTransformUpdated(
    USceneComponent* InRootComponent,
    EUpdateTransformFlags UpdateTransformFlags,
    ETeleportType Teleport) {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called HandleActorTransformUpdated on component %s"),
      *this->GetName());

  // When this notification was caused by the SetWorldTransform
  // call in _updateActorTransform, ignore it
  if (_updatingActorTransform) {
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT(
            "Ignoring HandleActorTransformUpdated, because it was triggered internally"));
    return;
  }

  _updateFromActor();
}

void UCesiumGeoreferenceComponent::_updateFromActor() {
  if (!IsValid(this->Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreferenceComponent does not have a valid Georeference"));
    return;
  }
  // Do NOT use TransformUnrealToEcef, because it will assume
  // that the given position is relative to the world origin
  const glm::dvec3 absoluteLocation = _getAbsoluteLocationFromActor();
  const glm::dmat4& unrealToEcef =
      this->Georeference->GetGeoTransforms()
          .GetAbsoluteUnrealWorldToEllipsoidCenteredTransform();
  const glm::dvec3 ecef = unrealToEcef * glm::dvec4(absoluteLocation, 1.0);

  _setECEF(ecef, true);
}

glm::dvec3 UCesiumGeoreferenceComponent::_getAbsoluteLocationFromActor() {
  const UWorld* world = this->GetWorld();
  if (!IsValid(world)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreferenceComponent %s is not spawned in world"),
        *this->GetName());
    return glm::dvec3();
  }
  const AActor* const owner = this->GetOwner();
  if (!IsValid(owner)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreferenceComponent %s does not have a valid owner"),
        *this->GetName());
    return glm::dvec3();
  }
  const glm::dvec3 worldOriginLocation =
      VecMath::createVector3D(world->OriginLocation);
  const USceneComponent* pOwnerRoot = owner->GetRootComponent();
  const FVector& relativeLocation = pOwnerRoot->GetComponentLocation();
  return worldOriginLocation + VecMath::createVector3D(relativeLocation);
}

glm::dmat3 UCesiumGeoreferenceComponent::_getRotationFromActor() {
  const AActor* owner = this->GetOwner();
  if (!IsValid(owner)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreferenceComponent %s does not have a valid owner"),
        *this->GetName());
    return glm::dmat3(1.0);
  }
  const USceneComponent* ownerRoot = owner->GetRootComponent();
  const FMatrix actorTransform =
      ownerRoot->GetComponentTransform().ToMatrixWithScale();
  const glm::dmat3 actorRotation(VecMath::createMatrix4D(actorTransform));
  return actorRotation;
}

void UCesiumGeoreferenceComponent::OnComponentCreated() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called OnComponentCreated on component %s"),
      *this->GetName());
  Super::OnComponentCreated();

  // When the component is first created, the ECEF position is unknown.
  // It will be computed in OnRegister if it hasn't been set or loaded prior to
  // that. This happens both on creation of a brand-new Component and on paste
  // of a Component into a new Actor.
  //
  // We could get rid of this if we instead has a way of _checking_ whether the
  // ECEF position is accurate. For example, is the ECEF position stored in this
  // component accurate for the Actor that the Component has just been attached
  // to? In other words, is it equivalent to the Actor's Location (to the limits
  // of the single-precision floating-point representation of the Location)?
  // Maybe because the Component was cut from this same Actor seconds ago? If
  // so, our ECEF is still valid. If not, we need to recompute the ECEF from the
  // (new) Actor transform.
  //
  // But just assuming the ECEF is invalid in this scenario is simpler and
  // nearly as good. It just means that on Paste we always recompute the ECEF
  // from the Actor Location.
  this->_ecefIsValid = false;
}

void UCesiumGeoreferenceComponent::PostLoad() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called PostLoad on component %s"),
      *this->GetName());
  Super::PostLoad();

  // TODO: do this in (de-)Serialize instead?
  this->_currentEcef = glm::dvec3(this->ECEF_X, this->ECEF_Y, this->ECEF_Z);
  this->_ecefIsValid = true;
}

void UCesiumGeoreferenceComponent::_initGeoreference() {
  if (!this->Georeference) {
    this->Georeference = ACesiumGeoreference::GetDefaultGeoreference(this);
  }
  if (this->Georeference) {
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT(
            "Attaching CesiumGeoreferenceComponent callback to Georeference %s"),
        *this->Georeference->GetFullName());
    this->Georeference->OnGeoreferenceUpdated.AddUniqueDynamic(
        this,
        &UCesiumGeoreferenceComponent::HandleGeoreferenceUpdated);
  }
}

void UCesiumGeoreferenceComponent::ApplyWorldOffset(
    const FVector& InOffset,
    bool bWorldShift) {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called ApplyWorldOffset on component %s"),
      *this->GetName());
  UActorComponent::ApplyWorldOffset(InOffset, bWorldShift);

  const UWorld* world = this->GetWorld();
  if (!IsValid(world)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreferenceComponent %s is not spawned in world"),
        *this->GetName());
    return;
  }
  if (!IsValid(this->Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "CesiumGeoreferenceComponent %s does not have a valid Georeference"),
        *this->GetName());
    return;
  }

  // Compute the position that the world origin will have
  // after the rebase, indeed by SUBTRACTING the offset
  const glm::dvec3 oldWorldOriginLocation =
      VecMath::createVector3D(world->OriginLocation);
  const glm::dvec3 offset = VecMath::createVector3D(InOffset);
  const glm::dvec3 newWorldOriginLocation = oldWorldOriginLocation - offset;

  // Compute the absolute location based on the ECEF
  // Do NOT use TransformEcefToUnreal, because it will return the
  // position relative to the current world origin!
  const glm::dmat4& ecefToUnreal =
      this->Georeference->GetGeoTransforms()
          .GetEllipsoidCenteredToAbsoluteUnrealWorldTransform();
  const glm::dvec3 absoluteLocation =
      ecefToUnreal * glm::dvec4(_currentEcef, 1.0);

  // Compute the new (high-precision) relative location from
  // the absolute location and the new world origin
  const glm::dvec3 newRelativeLocation =
      absoluteLocation - newWorldOriginLocation;

  const glm::dmat3 actorRotation = _getRotationFromActor();
  _updateActorTransform(actorRotation, newRelativeLocation);
}

#if WITH_EDITOR
void UCesiumGeoreferenceComponent::PostEditChangeProperty(
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
          GET_MEMBER_NAME_CHECKED(UCesiumGeoreferenceComponent, Longitude) ||
      propertyName ==
          GET_MEMBER_NAME_CHECKED(UCesiumGeoreferenceComponent, Latitude) ||
      propertyName ==
          GET_MEMBER_NAME_CHECKED(UCesiumGeoreferenceComponent, Height)) {
    this->MoveToLongitudeLatitudeHeight(
        glm::dvec3(this->Longitude, this->Latitude, this->Height));
    return;
  } else if (
      propertyName ==
          GET_MEMBER_NAME_CHECKED(UCesiumGeoreferenceComponent, ECEF_X) ||
      propertyName ==
          GET_MEMBER_NAME_CHECKED(UCesiumGeoreferenceComponent, ECEF_Y) ||
      propertyName ==
          GET_MEMBER_NAME_CHECKED(UCesiumGeoreferenceComponent, ECEF_Z)) {
    this->MoveToECEF(glm::dvec3(this->ECEF_X, this->ECEF_Y, this->ECEF_Z));
    return;
  }
}

void UCesiumGeoreferenceComponent::PreEditUndo() { Super::PreEditUndo(); }
void UCesiumGeoreferenceComponent::PostEditUndo() { Super::PostEditUndo(); }
#endif

void UCesiumGeoreferenceComponent::HandleGeoreferenceUpdated() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called HandleGeoreferenceUpdated for %s"),
      *this->GetName());

  if (!this->_ecefIsValid) {
    // We don't have a valid ECEF position, so no possible way to update the
    // Actor Transform based on the Georeference change.
    return;
  }

  if (!this->IsRegistered()) {
    // While this component is not registered, it is not in control of the
    // Actor's position. So a georeference change shouldn't affect the Actor's
    // transform. Furthermore, _currentUnrealToEcef is invalid when
    // unregistered, so we wouldn't know how to adjust the rotation.
    return;
  }

  const GeoTransforms& geoTransforms = this->Georeference->GetGeoTransforms();

  // Compute the change of rotation that is implied by the
  // new ECEF-to-Unreal rotation, and the one that was
  // stored previously
  const glm::dmat3 ecefToUnreal = glm::dmat3(
      geoTransforms.GetEllipsoidCenteredToAbsoluteUnrealWorldTransform());
  const glm::dmat3 rotationChange = ecefToUnreal * _currentUnrealToEcef;

  // Update the actor rotation based on the rotation change
  const glm::dmat3 oldActorRotation = _getRotationFromActor();
  const glm::dmat3 newActorRotation = rotationChange * oldActorRotation;

  // Store the new Unreal-to-ECEF rotation for further updates
  const glm::dmat3 unrealToEcef = glm::dmat3(
      geoTransforms.GetAbsoluteUnrealWorldToEllipsoidCenteredTransform());
  this->_currentUnrealToEcef = unrealToEcef;

  const glm::dvec3 relativeLocation = _computeRelativeLocation(_currentEcef);
  _updateActorTransform(newActorRotation, relativeLocation);
}

glm::dvec3
UCesiumGeoreferenceComponent::_computeRelativeLocation(const glm::dvec3& ecef) {
  const UWorld* const world = this->GetWorld();
  if (!IsValid(world)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreferenceComponent %s is not spawned in world"),
        *this->GetName());
    return glm::dvec3();
  }
  const AActor* const owner = this->GetOwner();
  if (!IsValid(owner)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreferenceComponent %s does not have a valid owner"),
        *this->GetName());
    return glm::dvec3();
  }
  // Compute the absolute location from the ECEF.
  // Do NOT use TransformEcefToUnreal, because it will return the
  // position relative to the current world origin!
  const glm::dmat4& ecefToUnreal =
      this->Georeference->GetGeoTransforms()
          .GetEllipsoidCenteredToAbsoluteUnrealWorldTransform();
  const glm::dvec3 absoluteLocation = ecefToUnreal * glm::dvec4(ecef, 1.0);

  // Compute the (high-precision) relative location
  // from the absolute location and the world origin
  const glm::dvec3 worldOriginLocation =
      VecMath::createVector3D(world->OriginLocation);
  const glm::dvec3 relativeLocation = absoluteLocation - worldOriginLocation;

  return relativeLocation;
}

glm::dvec3 UCesiumGeoreferenceComponent::_computeEllipsoidNormalUnreal(
    const glm::dvec3& ecef) {

  if (!IsValid(this->Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "CesiumGeoreferenceComponent %s does not have a valid Georeference"),
        *this->GetName());
    return glm::dvec3();
  }
  const glm::dvec3 ellipsoidNormalEcef =
      this->Georeference->ComputeGeodeticSurfaceNormal(ecef);
  const glm::dmat4& ecefToUnreal =
      this->Georeference->GetGeoTransforms()
          .GetEllipsoidCenteredToAbsoluteUnrealWorldTransform();
  const glm::dvec3 ellipsoidNormalUnreal =
      glm::dvec3(ecefToUnreal * glm::dvec4(ellipsoidNormalEcef, 0.0));
  return glm::normalize(ellipsoidNormalUnreal);
}

void UCesiumGeoreferenceComponent::_updateActorTransform() {
  const UWorld* const world = this->GetWorld();
  if (!IsValid(world)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreferenceComponent %s is not spawned in world"),
        *this->GetName());
    return;
  }
  const AActor* const owner = this->GetOwner();
  if (!IsValid(owner)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreferenceComponent %s does not have a valid owner"),
        *this->GetName());
    return;
  }

  const glm::dvec3 relativeLocation = _computeRelativeLocation(_currentEcef);
  const glm::dmat3 actorRotation = _getRotationFromActor();
  _updateActorTransform(actorRotation, relativeLocation);
}

void UCesiumGeoreferenceComponent::_updateActorTransform(
    const glm::dmat3& rotation,
    const glm::dvec3& translation) {
  const AActor* const owner = this->GetOwner();
  if (!IsValid(owner)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreferenceComponent %s does not have a valid owner"),
        *this->GetName());
    return;
  }
  if (_updatingActorTransform) {
    return;
  }
  _updatingActorTransform = true;

  // Create a matrix from the actor rotation, and set its
  // translational component to the high-precision
  // relative location
  glm::dmat4 actorToRelativeWorldHigh = glm::dmat4(rotation);
  actorToRelativeWorldHigh[3] = glm::dvec4(translation, 1.0);
  const FMatrix actorToRelativeWorld =
      VecMath::createMatrix(actorToRelativeWorldHigh);

  USceneComponent* const ownerRoot = owner->GetRootComponent();
  ownerRoot->SetWorldTransform(
      FTransform(actorToRelativeWorld),
      false,
      nullptr,
      this->TeleportWhenUpdatingTransform ? ETeleportType::TeleportPhysics
                                          : ETeleportType::None);
  _updatingActorTransform = false;
}

void UCesiumGeoreferenceComponent::_setECEF(
    const glm::dvec3& targetEcef,
    bool maintainRelativeOrientation) {

  GlmLogging::logVector("_setECEF _currentEcef ", _currentEcef);
  GlmLogging::logVector("_setECEF   targetEcef ", targetEcef);
  _debugLogState();

  if (!IsValid(this->Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "CesiumGeoreferenceComponent %s does not have a valid Georeference"),
        *this->GetName());
    return;
  }

  const glm::dmat3 oldActorRotation = _getRotationFromActor();
  glm::dmat3 newActorRotation = oldActorRotation;
  const glm::dvec3 newRelativeLocation = _computeRelativeLocation(targetEcef);

  if (!maintainRelativeOrientation || !this->_ecefIsValid) {
    // When NOT maintaining relative orientation, or we didn't previously know
    // our ECEF position, just update the ECEF position with the new values

    this->ECEF_X = targetEcef.x;
    this->ECEF_Y = targetEcef.y;
    this->ECEF_Z = targetEcef.z;
    this->_currentEcef = targetEcef;
  } else {
    // When maintaining the relative orientation, compute
    // the surface normal of the ellipsoid at the old and
    // the new position, and use the rotation between
    // these normals to update the actor rotation
    const glm::dvec3 oldEllipsoidNormalUnreal =
        _computeEllipsoidNormalUnreal(_currentEcef);
    const glm::dvec3 newEllipsoidNormalUnreal =
        _computeEllipsoidNormalUnreal(targetEcef);

    // The rotation between the old and the new normal
    const glm::dquat R =
        glm::rotation(oldEllipsoidNormalUnreal, newEllipsoidNormalUnreal);
    const glm::dmat3 alignmentRotation = glm::mat3_cast(R);

    // Compute the new actor rotation
    newActorRotation = alignmentRotation * oldActorRotation;

    this->ECEF_X = targetEcef.x;
    this->ECEF_Y = targetEcef.y;
    this->ECEF_Z = targetEcef.z;
    this->_currentEcef = targetEcef;
  }

  this->_ecefIsValid = true;
  _updateActorTransform(newActorRotation, newRelativeLocation);
  this->_updateDisplayLongitudeLatitudeHeight();

  GlmLogging::logVector("_setECEF done, _currentEcef now ", _currentEcef);
  _debugLogState();
}

void UCesiumGeoreferenceComponent::_updateDisplayLongitudeLatitudeHeight() {

  if (!IsValid(this->Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "CesiumGeoreferenceComponent %s does not have a valid Georeference"),
        *this->GetName());
    return;
  }
  const glm::dvec3 cartographic =
      this->Georeference->TransformEcefToLongitudeLatitudeHeight(_currentEcef);
  this->Longitude = cartographic.x;
  this->Latitude = cartographic.y;
  this->Height = cartographic.z;
}

void UCesiumGeoreferenceComponent::_debugLogState() {
  const UWorld* world = this->GetWorld();
  if (!IsValid(world)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreferenceComponent %s is not spawned in world"),
        *this->GetName());
    return;
  }
  const AActor* const owner = this->GetOwner();
  if (!IsValid(owner)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreferenceComponent %s does not have a valid owner"),
        *this->GetName());
    return;
  }
  if (!IsValid(this->Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "CesiumGeoreferenceComponent %s does not have a valid Georeference"),
        *this->GetName());
    return;
  }
  const glm::dmat4& ecefToAbsoluteUnreal =
      this->Georeference->GetGeoTransforms()
          .GetEllipsoidCenteredToAbsoluteUnrealWorldTransform();
  const glm::dvec3 absoluteLocation =
      ecefToAbsoluteUnreal * glm::dvec4(_currentEcef, 1.0);
  const glm::dvec3 worldOriginLocation =
      VecMath::createVector3D(world->OriginLocation);
  const glm::dvec3 relativeLocation = absoluteLocation - worldOriginLocation;

  const glm::dmat3 actorRotation = _getRotationFromActor();

  const USceneComponent* ownerRoot = owner->GetRootComponent();
  const FVector& componentLocation = ownerRoot->GetComponentLocation();
  const glm::dvec3 relativeLocationFromActor =
      VecMath::createVector3D(componentLocation);

  UE_LOG(LogCesium, Verbose, TEXT("State of %s"), *this->GetName());
  GlmLogging::logVector("  _currentEcef                ", _currentEcef);
  GlmLogging::logVector("  worldOriginLocation         ", worldOriginLocation);
  GlmLogging::logVector("  relativeLocation            ", relativeLocation);
  GlmLogging::logVector("  absoluteLocation            ", absoluteLocation);
  GlmLogging::logVector(
      "  relativeLocationFromActor   ",
      relativeLocationFromActor);
  GlmLogging::logMatrix("  actorRotation", glm::dmat4(actorRotation));
}
