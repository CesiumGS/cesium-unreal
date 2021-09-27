#include "CesiumGlobeAnchorComponent.h"
#include "CesiumActors.h"
#include "CesiumCustomVersion.h"
#include "CesiumGeoreference.h"
#include "CesiumTransforms.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "VecMath.h"
#include <glm/gtx/matrix_decompose.hpp>

ACesiumGeoreference* UCesiumGlobeAnchorComponent::GetGeoreference() const {
  return this->Georeference;
}

void UCesiumGlobeAnchorComponent::SetGeoreference(
    ACesiumGeoreference* NewGeoreference) {
  this->Georeference = NewGeoreference;
  this->InvalidateResolvedGeoreference();
  this->ResolveGeoreference();
}

glm::dvec3 UCesiumGlobeAnchorComponent::GetECEF() const {
  if (!this->_actorToECEFIsValid) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "CesiumGlobeAnchorComponent %s globe position is invalid because the component is not yet registered."),
        *this->GetName());
    return glm::dvec3(0.0);
  }

  return glm::dvec3(this->_actorToECEF[3]);
}

void UCesiumGlobeAnchorComponent::MoveToECEF(const glm::dvec3& newPosition) {
  this->ECEF_X = newPosition.x;
  this->ECEF_Y = newPosition.y;
  this->ECEF_Z = newPosition.z;
  this->_applyCartesianProperties();
}

void UCesiumGlobeAnchorComponent::InaccurateMoveToECEF(
    const FVector& TargetEcef) {
  this->MoveToECEF(VecMath::createVector3D(TargetEcef));
}

void UCesiumGlobeAnchorComponent::SnapLocalUpToEllipsoidNormal() {
  if (!this->_actorToECEFIsValid || !IsValid(this->ResolvedGeoreference)) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "CesiumGlobeAnchorComponent %s globe orientation cannot be changed because the component is not yet registered."),
        *this->GetName());
    return;
  }

  // Compute the current local up axis of the actor (the +Z axis)
  glm::dmat3 currentRotation = glm::dmat3(this->_actorToECEF);
  const glm::dvec3 actorUp = glm::normalize(currentRotation[2]);

  // Compute the surface normal of the ellipsoid
  const glm::dvec3 ellipsoidNormal =
      this->ResolvedGeoreference->ComputeGeodeticSurfaceNormal(this->GetECEF());

  // Find the shortest rotation to align local up with the ellipsoid normal
  const glm::dquat R = glm::rotation(actorUp, ellipsoidNormal);
  const glm::dmat3 alignmentRotation = glm::mat3_cast(R);

  // Compute the new actor rotation and apply it
  const glm::dmat3 newRotation = alignmentRotation * currentRotation;
  this->_actorToECEF = glm::dmat4(
      glm::dvec4(newRotation[0], 0.0),
      glm::dvec4(newRotation[1], 0.0),
      glm::dvec4(newRotation[2], 0.0),
      this->_actorToECEF[3]);

  this->_updateActorTransformFromGlobeTransform();
}

void UCesiumGlobeAnchorComponent::SnapToEastSouthUp() {
  if (!this->_actorToECEFIsValid || !IsValid(this->ResolvedGeoreference)) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "CesiumGlobeAnchorComponent %s globe orientation cannot be changed because the component is not yet registered."),
        *this->GetName());
    return;
  }

  // Extract the translation and scale from the existing transformation.
  // We assume there is no perspective or skew.
  glm::dvec4 translation = this->_actorToECEF[3];
  glm::dvec3 scale = glm::dvec3(
      glm::length(this->_actorToECEF[0]),
      glm::length(this->_actorToECEF[1]),
      glm::length(this->_actorToECEF[2]));

  // Compute the desired new orientation.
  glm::dmat3 newOrientation =
      this->ResolvedGeoreference->GetGeoTransforms().ComputeEastNorthUpToEcef(
          translation) *
      glm::dmat3(CesiumTransforms::unrealToOrFromCesium);

  // Scale the new orientation
  newOrientation[0] *= scale.x;
  newOrientation[1] *= scale.y;
  newOrientation[2] *= scale.z;

  // Recompose the transform.
  this->_actorToECEF = glm::dmat4(
      glm::dvec4(newOrientation[0], 0.0),
      glm::dvec4(newOrientation[1], 0.0),
      glm::dvec4(newOrientation[2], 0.0),
      translation);

  // Update the actor from the new globe transform
  this->_updateActorTransformFromGlobeTransform();
}

ACesiumGeoreference* UCesiumGlobeAnchorComponent::ResolveGeoreference() {
  if (IsValid(this->ResolvedGeoreference)) {
    return this->ResolvedGeoreference;
  }

  if (IsValid(this->Georeference)) {
    this->ResolvedGeoreference = this->Georeference;
  } else {
    this->ResolvedGeoreference =
        ACesiumGeoreference::GetDefaultGeoreference(this);
  }

  if (this->ResolvedGeoreference) {
    this->ResolvedGeoreference->OnGeoreferenceUpdated.AddUniqueDynamic(
        this,
        &UCesiumGlobeAnchorComponent::_onGeoreferenceChanged);
  }

  this->_onGeoreferenceChanged();

  return this->ResolvedGeoreference;
}

void UCesiumGlobeAnchorComponent::InvalidateResolvedGeoreference() {
  if (IsValid(this->ResolvedGeoreference)) {
    this->ResolvedGeoreference->OnGeoreferenceUpdated.RemoveAll(this);
  }
  this->ResolvedGeoreference = nullptr;
}

glm::dvec3 UCesiumGlobeAnchorComponent::GetLongitudeLatitudeHeight() const {
  if (!this->_actorToECEFIsValid || !this->ResolvedGeoreference) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "CesiumGlobeAnchorComponent %s globe position is invalid because the component is not yet registered."),
        *this->GetName());
    return glm::dvec3(0.0);
  }

  return this->ResolvedGeoreference->TransformEcefToLongitudeLatitudeHeight(
      this->GetECEF());
}

FVector
UCesiumGlobeAnchorComponent::InaccurateGetLongitudeLatitudeHeight() const {
  return VecMath::createVector(this->GetLongitudeLatitudeHeight());
}

void UCesiumGlobeAnchorComponent::MoveToLongitudeLatitudeHeight(
    const glm::dvec3& TargetLongitudeLatitudeHeight) {
  if (!this->_actorToECEFIsValid || !this->ResolvedGeoreference) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "CesiumGlobeAnchorComponent %s cannot move to a globe position because the component is not yet registered."),
        *this->GetName());
    return;
  }

  this->MoveToECEF(
      this->ResolvedGeoreference->TransformLongitudeLatitudeHeightToEcef(
          TargetLongitudeLatitudeHeight));
}

void UCesiumGlobeAnchorComponent::InaccurateMoveToLongitudeLatitudeHeight(
    const FVector& TargetLongitudeLatitudeHeight) {
  return this->MoveToLongitudeLatitudeHeight(
      VecMath::createVector3D(TargetLongitudeLatitudeHeight));
}

void UCesiumGlobeAnchorComponent::ApplyWorldOffset(
    const FVector& InOffset,
    bool bWorldShift) {
  // By the time this is called, all of the Actor's SceneComponents (including
  // its RootComponent) will already have had ApplyWorldOffset called on them.
  // So the root component's transform already reflects the shifted origin. It's
  // imprecise, though.
  //
  // Fortunately, this process does _not_ trigger the `TransformUpdated` event.
  // So our _actorToECEF transform still represents the precise globe transform
  // of the Actor.
  //
  // We simply need to convert the globe transform to a new Actor transform
  // based on the updated OriginLocation. The only slightly tricky part of this
  // is that the OriginLocation hasn't actually been updated yet.
  UActorComponent::ApplyWorldOffset(InOffset, bWorldShift);

  const UWorld* pWorld = this->GetWorld();
  if (!IsValid(pWorld)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGlobeAnchorComponent %s is not spawned in world"),
        *this->GetName());
    return;
  }

  // Compute the position that the world origin will have
  // after the rebase, indeed by SUBTRACTING the offset
  const glm::dvec3 oldWorldOriginLocation =
      VecMath::createVector3D(pWorld->OriginLocation);
  const glm::dvec3 offset = VecMath::createVector3D(InOffset);
  const glm::dvec3 newWorldOriginLocation = oldWorldOriginLocation - offset;

  // Update the Actor transform from the globe transform with the new origin
  // location explicitly provided.
  this->_updateActorTransformFromGlobeTransform(newWorldOriginLocation);
}

void UCesiumGlobeAnchorComponent::Serialize(FArchive& Ar) {
  Super::Serialize(Ar);

  Ar.UsingCustomVersion(FCesiumCustomVersion::GUID);

  const int32 CesiumVersion = Ar.CustomVer(FCesiumCustomVersion::GUID);

  if (CesiumVersion < FCesiumCustomVersion::GeoreferenceRefactoring) {
    // In previous versions, there was no _actorToECEFIsValid flag. But we can
    // assume that the previously-stored ECEF transform was valid.
    this->_actorToECEFIsValid = true;
  }
}

void UCesiumGlobeAnchorComponent::OnComponentCreated() {
  Super::OnComponentCreated();
  this->_actorToECEFIsValid = false;
}

#if WITH_EDITOR
void UCesiumGlobeAnchorComponent::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) {
  if (!PropertyChangedEvent.Property) {
    return;
  }

  FName propertyName = PropertyChangedEvent.Property->GetFName();

  if (propertyName ==
          GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorComponent, Longitude) ||
      propertyName ==
          GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorComponent, Latitude) ||
      propertyName ==
          GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorComponent, Height)) {
    this->_applyCartographicProperties();
  } else if (
      propertyName ==
          GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorComponent, ECEF_X) ||
      propertyName ==
          GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorComponent, ECEF_Y) ||
      propertyName ==
          GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorComponent, ECEF_Z)) {
    this->_applyCartesianProperties();
  } else if (
      propertyName ==
      GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorComponent, Georeference)) {
    this->InvalidateResolvedGeoreference();
  }
}
#endif

void UCesiumGlobeAnchorComponent::OnRegister() {
  Super::OnRegister();

  const AActor* pOwner = this->GetOwner();
  if (!IsValid(pOwner)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGlobeAnchorComponent %s does not have a valid owner"),
        *this->GetName());
    return;
  }

  USceneComponent* pOwnerRoot = pOwner->GetRootComponent();
  if (pOwnerRoot) {
    pOwnerRoot->TransformUpdated.AddUObject(
        this,
        &UCesiumGlobeAnchorComponent::_onActorTransformChanged);
  }

  // Resolve the georeference, which will also subscribe to the new georeference
  // (if there is one) and call _onGeoreferenceChanged.
  // This will update the actor transform with the globe position, but only if
  // the globe transform is valid.
  this->ResolveGeoreference();

  // If the globe transform is not yet valid, compute it from the actor
  // transform now.
  if (!this->_actorToECEFIsValid) {
    this->_updateGlobeTransformFromActorTransform();
  }
}

void UCesiumGlobeAnchorComponent::OnUnregister() {
  Super::OnUnregister();

  // Unsubscribe from the ResolvedGeoreference.
  this->InvalidateResolvedGeoreference();

  // Unsubscribe from the TransformUpdated event.
  const AActor* pOwner = this->GetOwner();
  if (!IsValid(pOwner)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGlobeAnchorComponent %s does not have a valid owner"),
        *this->GetName());
    return;
  }

  USceneComponent* pOwnerRoot = pOwner->GetRootComponent();
  if (pOwnerRoot) {
    pOwnerRoot->TransformUpdated.RemoveAll(this);
  }
}

void UCesiumGlobeAnchorComponent::_onActorTransformChanged(
    USceneComponent* InRootComponent,
    EUpdateTransformFlags UpdateTransformFlags,
    ETeleportType Teleport) {
  if (this->_updatingActorTransform) {
    return;
  }

  if (!this->ResolvedGeoreference) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "CesiumGlobeAnchorComponent %s cannot update globe transform from actor transform because there is no valid Georeference."),
        *this->GetName());
    return;
  }

  if (!this->_actorToECEFIsValid ||
      !this->AdjustOrientationForGlobeWhenMoving) {
    // We can't or don't want to adjust the orientation, so just compute the new
    // globe transform.
    this->_updateGlobeTransformFromActorTransform();
    return;
  }

  // Also adjust the orientation so that the Object is still "upright" at the
  // new position on the globe.

  // Store the old globe position and compute the new transform.
  const glm::dvec3 oldGlobePosition = glm::dvec3(this->_actorToECEF[3]);
  const glm::dmat4& newGlobeTransform =
      this->_updateGlobeTransformFromActorTransform();

  // Compute the surface normal rotation between the old and new positions.
  const glm::dvec3 newGlobePosition = glm::dvec3(newGlobeTransform[3]);
  const glm::dquat ellipsoidNormalRotation =
      this->ResolvedGeoreference->GetGeoTransforms()
          .ComputeSurfaceNormalRotationUnreal(
              oldGlobePosition,
              newGlobePosition);

  // Adjust the new rotation by the surface normal rotation
  const glm::dquat rotation = VecMath::createQuaternion(
      InRootComponent->GetComponentRotation().Quaternion());
  const glm::dquat adjustedRotation = ellipsoidNormalRotation * rotation;

  // Set the new Actor transform, taking care not to do this recursively.
  this->_updatingActorTransform = true;
  InRootComponent->SetWorldRotation(
      VecMath::createQuaternion(adjustedRotation),
      false,
      nullptr,
      this->TeleportWhenUpdatingTransform ? ETeleportType::TeleportPhysics
                                          : ETeleportType::None);
  this->_updatingActorTransform = false;

  // Compute the globe transform from the updated Actor transform.
  this->_updateGlobeTransformFromActorTransform();
}

void UCesiumGlobeAnchorComponent::_onGeoreferenceChanged() {
  if (this->_actorToECEFIsValid) {
    this->_updateActorTransformFromGlobeTransform();
  }
}

const glm::dmat4&
UCesiumGlobeAnchorComponent::_updateGlobeTransformFromActorTransform() {
  if (!this->ResolvedGeoreference) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "CesiumGlobeAnchorComponent %s cannot update globe transform from actor transform because there is no valid Georeference."),
        *this->GetName());
    this->_actorToECEFIsValid = false;
    return this->_actorToECEF;
  }

  const AActor* pOwner = this->GetOwner();
  if (!IsValid(pOwner)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("UCesiumGlobeAnchorComponent %s does not have a valid owner"),
        *this->GetName());
    this->_actorToECEFIsValid = false;
    return this->_actorToECEF;
  }

  const USceneComponent* pOwnerRoot = pOwner->GetRootComponent();
  if (!IsValid(pOwnerRoot)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "The owner of UCesiumGlobeAnchorComponent %s does not have a valid root component"),
        *this->GetName());
    this->_actorToECEFIsValid = false;
    return this->_actorToECEF;
  }

  // Get the relative world transform.
  glm::dmat4 actorTransform = VecMath::createMatrix4D(
      pOwnerRoot->GetComponentTransform().ToMatrixWithScale());

  // Convert to an absolute world transform
  actorTransform[3] += CesiumActors::getWorldOrigin4D(pOwner);
  actorTransform[3].w = 1.0;

  // Convert to ECEF
  const glm::dmat4& absoluteUnrealToEcef =
      this->ResolvedGeoreference->GetGeoTransforms()
          .GetAbsoluteUnrealWorldToEllipsoidCenteredTransform();

  this->_actorToECEF = absoluteUnrealToEcef * actorTransform;
  this->_actorToECEFIsValid = true;

  this->_updateCartesianProperties();
  this->_updateCartographicProperties();

  return this->_actorToECEF;
}

FTransform UCesiumGlobeAnchorComponent::_updateActorTransformFromGlobeTransform(
    const std::optional<glm::dvec3>& newWorldOrigin) {
  const AActor* pOwner = this->GetOwner();
  if (!IsValid(pOwner)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("UCesiumGlobeAnchorComponent %s does not have a valid owner"),
        *this->GetName());
    return FTransform();
  }

  USceneComponent* pOwnerRoot = pOwner->GetRootComponent();
  if (!IsValid(pOwnerRoot)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "The owner of UCesiumGlobeAnchorComponent %s does not have a valid root component"),
        *this->GetName());
    return FTransform();
  }

  if (!this->_actorToECEFIsValid) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "UCesiumGlobeAnchorComponent %s cannot update Actor transform from Globe transform because the Globe transform is not known."),
        *this->GetName());
    return pOwnerRoot->GetComponentTransform();
  }

  const GeoTransforms& geoTransforms =
      this->ResolveGeoreference()->GetGeoTransforms();

  // Transform ECEF to UE absolute world
  const glm::dmat4& ecefToAbsoluteUnreal =
      geoTransforms.GetEllipsoidCenteredToAbsoluteUnrealWorldTransform();
  glm::dmat4 actorToUnreal = ecefToAbsoluteUnreal * this->_actorToECEF;

  // Transform UE absolute world to UE relative world
  actorToUnreal[3] -= newWorldOrigin ? glm::dvec4(*newWorldOrigin, 1.0)
                                     : CesiumActors::getWorldOrigin4D(pOwner);
  actorToUnreal[3].w = 1.0;

  FTransform actorTransform = FTransform(VecMath::createMatrix(actorToUnreal));

  // Set the Actor transform
  this->_updatingActorTransform = true;
  pOwnerRoot->SetWorldTransform(
      actorTransform,
      false,
      nullptr,
      this->TeleportWhenUpdatingTransform ? ETeleportType::TeleportPhysics
                                          : ETeleportType::None);
  this->_updatingActorTransform = false;

  return actorTransform;
}

const glm::dmat4& UCesiumGlobeAnchorComponent::_setGlobeTransform(
    const glm::dmat4& newTransform) {
  // If we don't yet know our globe transform, we can't update the orientation
  // for globe curvature, so just replace the globe transform and we're done.
  // Do the same if we don't want to update the orientation for globe curvature.
  if (!this->_actorToECEFIsValid ||
      !this->AdjustOrientationForGlobeWhenMoving) {
    this->_actorToECEF = newTransform;
    this->_updateActorTransformFromGlobeTransform();
    return this->_actorToECEF;
  }

  // Save the old position and apply the new transform.
  const glm::dvec3 oldPosition = glm::dvec3(this->_actorToECEF[3]);
  const glm::dvec3 newPosition = glm::dvec3(newTransform[3]);

  // Adjust the orientation so that the Object is still "upright" at the new
  // position on the globe.

  // Compute the surface normal rotation between the old and new positions.
  const glm::dquat ellipsoidNormalRotation =
      this->ResolveGeoreference()
          ->GetGeoTransforms()
          .ComputeSurfaceNormalRotation(oldPosition, newPosition);

  // Adjust the new rotation by the surface normal rotation
  glm::dmat3 newRotation =
      glm::mat3_cast(ellipsoidNormalRotation) * glm::dmat3(newTransform);
  this->_actorToECEF = glm::dmat4(
      glm::dvec4(newRotation[0], 0.0),
      glm::dvec4(newRotation[1], 0.0),
      glm::dvec4(newRotation[2], 0.0),
      glm::dvec4(newPosition, 1.0));

  // Update the Actor transform from the new globe transform.
  this->_updateActorTransformFromGlobeTransform();

  return this->_actorToECEF;
}

void UCesiumGlobeAnchorComponent::_applyCartesianProperties() {
  // If we don't yet know our globe transform, compute it from the Actor
  // transform now.
  if (!this->_actorToECEFIsValid) {
    this->_updateGlobeTransformFromActorTransform();
  }

  glm::dmat4 transform = this->_actorToECEF;
  transform[3] = glm::dvec4(this->ECEF_X, this->ECEF_Y, this->ECEF_Z, 1.0);
  this->_setGlobeTransform(transform);

  this->_updateCartographicProperties();
}

void UCesiumGlobeAnchorComponent::_updateCartesianProperties() {
  if (!this->_actorToECEFIsValid) {
    return;
  }

  this->ECEF_X = this->_actorToECEF[3].x;
  this->ECEF_Y = this->_actorToECEF[3].y;
  this->ECEF_Z = this->_actorToECEF[3].z;
}

void UCesiumGlobeAnchorComponent::_applyCartographicProperties() {
  // If we don't yet know our globe transform, compute it from the Actor
  // transform now.
  if (!this->_actorToECEFIsValid) {
    this->_updateGlobeTransformFromActorTransform();
  }

  ACesiumGeoreference* pGeoreference = this->ResolveGeoreference();
  if (!pGeoreference) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "The UCesiumGlobeAnchorComponent %s does not have a valid Georeference"),
        *this->GetName());
  }

  glm::dmat4 transform = this->_actorToECEF;
  glm::dvec3 newEcef =
      pGeoreference->GetGeoTransforms().TransformLongitudeLatitudeHeightToEcef(
          glm::dvec3(this->Longitude, this->Latitude, this->Height));
  transform[3] = glm::dvec4(newEcef, 1.0);
  this->_setGlobeTransform(transform);

  this->_updateCartesianProperties();
}

void UCesiumGlobeAnchorComponent::_updateCartographicProperties() {
  if (!this->_actorToECEFIsValid) {
    return;
  }

  ACesiumGeoreference* pGeoreference = this->ResolveGeoreference();
  if (!pGeoreference) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "The UCesiumGlobeAnchorComponent %s does not have a valid Georeference"),
        *this->GetName());
  }

  glm::dvec3 llh =
      pGeoreference->GetGeoTransforms().TransformEcefToLongitudeLatitudeHeight(
          glm::dvec3(this->_actorToECEF[3]));

  this->Longitude = llh.x;
  this->Latitude = llh.y;
  this->Height = llh.z;
}
