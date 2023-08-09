// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGlobeAnchorComponent.h"
#include "CesiumCustomVersion.h"
#include "CesiumGeometry/Transforms.h"
#include "CesiumGeoreference.h"
#include "CesiumRuntime.h"
#include "CesiumWgs84Ellipsoid.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "VecMath.h"
#include <glm/gtx/quaternion.hpp>

// These are the "changes" that can happen to this component, how it detects
// them, and what it does about them:
//
// ## Actor Transform Changed
//
// * Detected by subscribing to the `TransformUpdated` event of the root
// component of the Actor to which this component is attached. The subscription
// is added in `OnRegister` and removed in `OnUnregister`.
// * Updates the ECEF transform from the new Actor transform.
// * If `AdjustOrientationForGlobeWhenMoving` is enabled, also applies a
// rotation based on the change in surface normal.
//
// ## Globe (ECEF) Position Changed
//
// * Happens when MoveToECEF (or similar) is called explicitly, or position
// properties are changed in the Editor.
// * Updates the Actor transform from the new ECEF transform.
// * If `AdjustOrientationForGlobeWhenMoving` is enabled, also applies a
// rotation based on the change in surface normal.
//
// ## Georeference Changed
//
// * Detected by subscribing to the `GeoreferenceUpdated` event. The
// subscription is added when a new Georeference is resolved in
// `ResolveGeoreference` (in `OnRegister` at the latest) and removed in
// `InvalidateResolvedGeoreference` (in `OnUnregister` and when the
// Georeference property is changed).
// * Updates the Actor transform from the existing ECEF transform.
// * Ignores `AdjustOrientationForGlobeWhenMoving` because the globe position is
// not changing.
//
// ## Origin Rebased
//
// * Detected by a call to `ApplyWorldOffset`.
// * Updates the Actor transform from the existing ECEF transform.
// * Ignores `AdjustOrientationForGlobeWhenMoving` because the globe position is
// not changing.

TSoftObjectPtr<ACesiumGeoreference>
UCesiumGlobeAnchorComponent::GetGeoreference() const {
  return this->Georeference;
}

void UCesiumGlobeAnchorComponent::SetGeoreference(
    TSoftObjectPtr<ACesiumGeoreference> NewGeoreference) {
  this->Georeference = NewGeoreference;
  this->InvalidateResolvedGeoreference();
  this->ResolveGeoreference();
}

FVector
UCesiumGlobeAnchorComponent::GetEarthCenteredEarthFixedPosition() const {
  if (!this->_actorToECEFIsValid) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "CesiumGlobeAnchorComponent %s globe position is invalid because the component is not yet registered."),
        *this->GetName());
    return FVector(0.0);
  }

  return this->ActorToEarthCenteredEarthFixedMatrix.GetOrigin();
}

FMatrix
UCesiumGlobeAnchorComponent::GetActorToEarthCenteredEarthFixedMatrix() const {
  if (!this->_actorToECEFIsValid) {
    const_cast<UCesiumGlobeAnchorComponent*>(this)
        ->_setNewActorToECEFFromRelativeTransform();
  }

  return this->ActorToEarthCenteredEarthFixedMatrix;
}

void UCesiumGlobeAnchorComponent::SetActorToEarthCenteredEarthFixedMatrix(
    const FMatrix& Value) {
  // This method is equivalent to
  // CesiumGlobeAnchorImpl::SetNewLocalToGlobeFixedMatrix in Cesium for Unity.

  if (!this->ResolvedGeoreference) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "CesiumGlobeAnchorComponent %s cannot set new Actor-to-ECEF transform because there is no valid CesiumGeoreference."),
        *this->GetName());
    return;
  }

  USceneComponent* pOwnerRoot = this->_getRootComponent(/*warnIfNull*/ true);
  if (!IsValid(pOwnerRoot)) {
    return;
  }

  // Update with the new ECEF transform, also rotating based on the new position
  // if desired.
  CesiumGeospatial::GlobeAnchor nativeAnchor =
      this->_createOrUpdateNativeGlobeAnchorFromECEF(Value);
  this->_updateFromNativeGlobeAnchor(nativeAnchor);

#if WITH_EDITOR
  // In the Editor, mark this component and the root component modified so Undo
  // works properly.
  this->Modify();
  pOwnerRoot->Modify();
#endif
}

bool UCesiumGlobeAnchorComponent::GetTeleportWhenUpdatingTransform() const {
  return this->TeleportWhenUpdatingTransform;
}

void UCesiumGlobeAnchorComponent::SetTeleportWhenUpdatingTransform(bool Value) {
  this->TeleportWhenUpdatingTransform = Value;
}

bool UCesiumGlobeAnchorComponent::GetAdjustOrientationForGlobeWhenMoving()
    const {
  return this->AdjustOrientationForGlobeWhenMoving;
}

void UCesiumGlobeAnchorComponent::SetAdjustOrientationForGlobeWhenMoving(
    bool Value) {
  this->AdjustOrientationForGlobeWhenMoving = Value;
}

void UCesiumGlobeAnchorComponent::MoveToEarthCenteredEarthFixedPosition(
    const FVector& TargetEcef) {
  if (!this->_actorToECEFIsValid)
    this->_setNewActorToECEFFromRelativeTransform();
  FMatrix newMatrix = this->ActorToEarthCenteredEarthFixedMatrix;
  newMatrix.SetOrigin(TargetEcef);
  this->SetActorToEarthCenteredEarthFixedMatrix(newMatrix);
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

  // Compute the current local up axis of the actor (the +Z axis) in ECEF
  FVector up = this->ActorToEarthCenteredEarthFixedMatrix.GetUnitAxis(EAxis::Z);

  // Compute the surface normal of the ellipsoid
  FVector ellipsoidNormal = UCesiumWgs84Ellipsoid::GeodeticSurfaceNormal(
      this->GetEarthCenteredEarthFixedPosition());

  // Find the shortest rotation to align local up with the ellipsoid normal
  FMatrix alignmentRotation =
      FQuat::FindBetween(up, ellipsoidNormal).ToMatrix();

  // Compute the new actor rotation and apply it
  FMatrix newActorToECEF =
      alignmentRotation * this->ActorToEarthCenteredEarthFixedMatrix;
  this->_updateFromNativeGlobeAnchor(
      this->_createNativeGlobeAnchor(newActorToECEF));

#if WITH_EDITOR
  // In the Editor, mark this component modified so Undo works properly.
  this->Modify();
#endif
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

  this->SetEastSouthUpRotation(FQuat::Identity);

#if WITH_EDITOR
  // In the Editor, mark this component modified so Undo works properly.
  this->Modify();
#endif
}

ACesiumGeoreference* UCesiumGlobeAnchorComponent::ResolveGeoreference() {
  if (IsValid(this->ResolvedGeoreference)) {
    return this->ResolvedGeoreference;
  }

  if (IsValid(this->Georeference.Get())) {
    this->ResolvedGeoreference = this->Georeference.Get();
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

FVector UCesiumGlobeAnchorComponent::GetLongitudeLatitudeHeight() const {
  if (!this->_actorToECEFIsValid || !this->ResolvedGeoreference) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "CesiumGlobeAnchorComponent %s globe position is invalid because the component is not yet registered."),
        *this->GetName());
    return FVector(0.0);
  }

  return UCesiumWgs84Ellipsoid::
      EarthCenteredEarthFixedToLongitudeLatitudeHeight(
          this->GetEarthCenteredEarthFixedPosition());
}

void UCesiumGlobeAnchorComponent::MoveToLongitudeLatitudeHeight(
    const FVector& TargetLongitudeLatitudeHeight) {
  if (!this->_actorToECEFIsValid || !this->ResolvedGeoreference) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "CesiumGlobeAnchorComponent %s cannot move to a globe position because the component is not yet registered."),
        *this->GetName());
    return;
  }

  this->MoveToEarthCenteredEarthFixedPosition(
      UCesiumWgs84Ellipsoid::LongitudeLatitudeHeightToEarthCenteredEarthFixed(
          TargetLongitudeLatitudeHeight));
}

namespace {

CesiumGeospatial::LocalHorizontalCoordinateSystem
createEastSouthUp(const CesiumGeospatial::GlobeAnchor& anchor) {
  glm::dvec3 ecefPosition;
  CesiumGeometry::Transforms::computeTranslationRotationScaleFromMatrix(
      anchor.getAnchorToFixedTransform(),
      &ecefPosition,
      nullptr,
      nullptr);

  return CesiumGeospatial::LocalHorizontalCoordinateSystem(
      ecefPosition,
      CesiumGeospatial::LocalDirection::East,
      CesiumGeospatial::LocalDirection::South,
      CesiumGeospatial::LocalDirection::Up,
      1.0);
}

} // namespace

FQuat UCesiumGlobeAnchorComponent::GetEastSouthUpRotation() const {
  CesiumGeospatial::GlobeAnchor anchor(
      VecMath::createMatrix4D(this->ActorToEarthCenteredEarthFixedMatrix));

  CesiumGeospatial::LocalHorizontalCoordinateSystem eastSouthUp =
      createEastSouthUp(anchor);

  glm::dmat4 modelToEastSouthUp = anchor.getAnchorToLocalTransform(eastSouthUp);

  glm::dquat rotationToEastSouthUp;
  CesiumGeometry::Transforms::computeTranslationRotationScaleFromMatrix(
      modelToEastSouthUp,
      nullptr,
      &rotationToEastSouthUp,
      nullptr);
  return VecMath::createQuaternion(rotationToEastSouthUp);
}

void UCesiumGlobeAnchorComponent::SetEastSouthUpRotation(
    const FQuat& EastSouthUpRotation) {
  CesiumGeospatial::GlobeAnchor anchor(
      VecMath::createMatrix4D(this->ActorToEarthCenteredEarthFixedMatrix));

  CesiumGeospatial::LocalHorizontalCoordinateSystem eastSouthUp =
      createEastSouthUp(anchor);

  glm::dmat4 modelToEastSouthUp = anchor.getAnchorToLocalTransform(eastSouthUp);

  glm::dvec3 translation;
  glm::dvec3 scale;
  CesiumGeometry::Transforms::computeTranslationRotationScaleFromMatrix(
      modelToEastSouthUp,
      &translation,
      nullptr,
      &scale);

  glm::dmat4 newModelToEastSouthUp =
      CesiumGeometry::Transforms::createTranslationRotationScaleMatrix(
          translation,
          VecMath::createQuaternion(EastSouthUpRotation),
          scale);

  anchor.setAnchorToLocalTransform(eastSouthUp, newModelToEastSouthUp, false);
  this->_updateFromNativeGlobeAnchor(anchor);
}

FQuat UCesiumGlobeAnchorComponent::GetEarthCenteredEarthFixedRotation() const {
  glm::dquat rotationToEarthCenteredEarthFixed;
  CesiumGeometry::Transforms::computeTranslationRotationScaleFromMatrix(
      VecMath::createMatrix4D(this->ActorToEarthCenteredEarthFixedMatrix),
      nullptr,
      &rotationToEarthCenteredEarthFixed,
      nullptr);
  return VecMath::createQuaternion(rotationToEarthCenteredEarthFixed);
}

void UCesiumGlobeAnchorComponent::SetEarthCenteredEarthFixedRotation(
    const FQuat& EarthCenteredEarthFixedRotation) {
  glm::dvec3 translation;
  glm::dvec3 scale;
  CesiumGeometry::Transforms::computeTranslationRotationScaleFromMatrix(
      VecMath::createMatrix4D(this->ActorToEarthCenteredEarthFixedMatrix),
      &translation,
      nullptr,
      &scale);

  glm::dmat4 newModelToEarthCenteredEarthFixed =
      CesiumGeometry::Transforms::createTranslationRotationScaleMatrix(
          translation,
          VecMath::createQuaternion(EarthCenteredEarthFixedRotation),
          scale);

  this->ActorToEarthCenteredEarthFixedMatrix =
      VecMath::createMatrix(newModelToEarthCenteredEarthFixed);
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

#if WITH_EDITORONLY_DATA
  if (CesiumVersion <
      FCesiumCustomVersion::GlobeAnchorTransformationAsFMatrix) {
    memcpy(
        this->ActorToEarthCenteredEarthFixedMatrix.M,
        this->_actorToECEF_Array_DEPRECATED,
        sizeof(double) * 16);
  }
#endif
}

void UCesiumGlobeAnchorComponent::OnComponentCreated() {
  Super::OnComponentCreated();
  this->_actorToECEFIsValid = false;
}

#if WITH_EDITOR
void UCesiumGlobeAnchorComponent::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) {
  Super::PostEditChangeProperty(PropertyChangedEvent);

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
    this->MoveToLongitudeLatitudeHeight(
        FVector(this->Longitude, this->Latitude, this->Height));
  } else if (
      propertyName ==
          GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorComponent, ECEF_X) ||
      propertyName ==
          GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorComponent, ECEF_Y) ||
      propertyName ==
          GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorComponent, ECEF_Z)) {
    this->MoveToEarthCenteredEarthFixedPosition(
        FVector(this->ECEF_X, this->ECEF_Y, this->ECEF_Z));
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
    if (_onTransformChangedWhileUnregistered.IsValid()) {
      pOwnerRoot->TransformUpdated.Remove(_onTransformChangedWhileUnregistered);
    }
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
    this->_setNewActorToECEFFromRelativeTransform();
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
    _onTransformChangedWhileUnregistered =
        pOwnerRoot->TransformUpdated.AddLambda(
            [this](USceneComponent*, EUpdateTransformFlags, ETeleportType) {
              this->_actorToECEFIsValid = false;
            });
  }
}

CesiumGeospatial::GlobeAnchor
UCesiumGlobeAnchorComponent::_createNativeGlobeAnchor() const {
  return this->_createNativeGlobeAnchor(
      this->ActorToEarthCenteredEarthFixedMatrix);
}

CesiumGeospatial::GlobeAnchor
UCesiumGlobeAnchorComponent::_createNativeGlobeAnchor(
    const FMatrix& actorToECEF) const {
  return CesiumGeospatial::GlobeAnchor(VecMath::createMatrix4D(actorToECEF));
}

USceneComponent*
UCesiumGlobeAnchorComponent::_getRootComponent(bool warnIfNull) const {
  const AActor* pOwner = this->GetOwner();
  if (!IsValid(pOwner)) {
    if (warnIfNull) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("UCesiumGlobeAnchorComponent %s does not have a valid owner."),
          *this->GetName());
    }
    return nullptr;
  }

  USceneComponent* pOwnerRoot = pOwner->GetRootComponent();
  if (!IsValid(pOwnerRoot)) {
    if (warnIfNull) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "The owner of UCesiumGlobeAnchorComponent %s does not have a valid root component."),
          *this->GetName());
    }
    return nullptr;
  }

  return pOwnerRoot;
}

FTransform UCesiumGlobeAnchorComponent::_getCurrentRelativeTransform() const {
  const USceneComponent* pOwnerRoot = this->_getRootComponent(true);
  return pOwnerRoot->GetRelativeTransform();
}

void UCesiumGlobeAnchorComponent::_setCurrentRelativeTransform(
    const FTransform& relativeTransform) {
  AActor* pOwner = this->GetOwner();
  if (!IsValid(pOwner)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("UCesiumGlobeAnchorComponent %s does not have a valid owner"),
        *this->GetName());
    return;
  }

  USceneComponent* pOwnerRoot = pOwner->GetRootComponent();
  if (!IsValid(pOwnerRoot)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "The owner of UCesiumGlobeAnchorComponent %s does not have a valid root component"),
        *this->GetName());
    return;
  }

  // Set the new Actor relative transform, taking care not to do this
  // recursively.
  this->_updatingActorTransform = true;
  pOwnerRoot->SetRelativeTransform(
      relativeTransform,
      false,
      nullptr,
      this->TeleportWhenUpdatingTransform ? ETeleportType::TeleportPhysics
                                          : ETeleportType::None);
  this->_updatingActorTransform = false;
}

CesiumGeospatial::GlobeAnchor UCesiumGlobeAnchorComponent::
    _createOrUpdateNativeGlobeAnchorFromRelativeTransform(
        const FTransform& newRelativeTransform) {
  ACesiumGeoreference* pGeoreference = this->ResolvedGeoreference;
  assert(pGeoreference != nullptr);

  const CesiumGeospatial::LocalHorizontalCoordinateSystem& local =
      pGeoreference->getCoordinateSystem();

  glm::dmat4 newModelToLocal =
      VecMath::createMatrix4D(newRelativeTransform.ToMatrixWithScale());

  if (!this->_actorToECEFIsValid) {
    // Create a new anchor initialized at the new position, because there is no
    // old one.
    return CesiumGeospatial::GlobeAnchor::fromAnchorToLocalTransform(
        local,
        newModelToLocal);
  } else {
    // Create an anchor at the old position and move it to the new one.
    CesiumGeospatial::GlobeAnchor cppAnchor = this->_createNativeGlobeAnchor();
    cppAnchor.setAnchorToLocalTransform(
        local,
        newModelToLocal,
        this->AdjustOrientationForGlobeWhenMoving);
    return cppAnchor;
  }
}

CesiumGeospatial::GlobeAnchor
UCesiumGlobeAnchorComponent::_createOrUpdateNativeGlobeAnchorFromECEF(
    const FMatrix& newActorToECEFMatrix) {
  if (!this->_actorToECEFIsValid) {
    // Create a new anchor initialized at the new position, because there is no
    // old one.
    return CesiumGeospatial::GlobeAnchor(
        VecMath::createMatrix4D(newActorToECEFMatrix));
  } else {
    // Create an anchor at the old position and move it to the new one.
    CesiumGeospatial::GlobeAnchor cppAnchor(
        VecMath::createMatrix4D(this->ActorToEarthCenteredEarthFixedMatrix));
    cppAnchor.setAnchorToFixedTransform(
        VecMath::createMatrix4D(newActorToECEFMatrix),
        this->AdjustOrientationForGlobeWhenMoving);
    return cppAnchor;
  }
}

void UCesiumGlobeAnchorComponent::_updateFromNativeGlobeAnchor(
    const CesiumGeospatial::GlobeAnchor& nativeAnchor) {
  this->ActorToEarthCenteredEarthFixedMatrix =
      VecMath::createMatrix(nativeAnchor.getAnchorToFixedTransform());
  this->_actorToECEFIsValid = true;

  // Update the editable position properties
  // TODO: it'd be nice if we didn't have to store these at all. But then we'd
  // need a custom UI to make them directly editable, I think.
  FVector origin = this->ActorToEarthCenteredEarthFixedMatrix.GetOrigin();
  this->ECEF_X = origin.X;
  this->ECEF_Y = origin.Y;
  this->ECEF_Z = origin.Z;

  FVector llh =
      UCesiumWgs84Ellipsoid::EarthCenteredEarthFixedToLongitudeLatitudeHeight(
          origin);
  this->Longitude = llh.X;
  this->Latitude = llh.Y;
  this->Height = llh.Z;

  // Update the Unreal relative transform
  ACesiumGeoreference* pGeoreference = this->ResolvedGeoreference;
  assert(pGeoreference != nullptr);

  glm::dmat4 anchorToLocal = nativeAnchor.getAnchorToLocalTransform(
      pGeoreference->getCoordinateSystem());

  this->_setCurrentRelativeTransform(
      FTransform(VecMath::createMatrix(anchorToLocal)));
}

void UCesiumGlobeAnchorComponent::_onActorTransformChanged(
    USceneComponent* InRootComponent,
    EUpdateTransformFlags UpdateTransformFlags,
    ETeleportType Teleport) {
  if (this->_updatingActorTransform) {
    return;
  }

  this->_setNewActorToECEFFromRelativeTransform();
}

void UCesiumGlobeAnchorComponent::_setNewActorToECEFFromRelativeTransform() {
  // This method is equivalent to
  // CesiumGlobeAnchorImpl::SetNewLocalToGlobeFixedMatrixFromTransform in Cesium
  // for Unity.

  if (!this->ResolvedGeoreference) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "CesiumGlobeAnchorComponent %s cannot update globe transform from actor transform because there is no valid Georeference."),
        *this->GetName());
    return;
  }

  USceneComponent* pOwnerRoot = this->_getRootComponent(/*warnIfNull*/ true);
  if (!IsValid(pOwnerRoot)) {
    return;
  }

  // Update with the new local transform, also rotating based on the new
  // position if desired.
  FTransform modelToLocal = this->_getCurrentRelativeTransform();
  CesiumGeospatial::GlobeAnchor cppAnchor =
      this->_createOrUpdateNativeGlobeAnchorFromRelativeTransform(modelToLocal);
  this->_updateFromNativeGlobeAnchor(cppAnchor);

#if WITH_EDITOR
  // In the Editor, mark this component and the root component modified so Undo
  // works properly.
  this->Modify();
  pOwnerRoot->Modify();
#endif
}

void UCesiumGlobeAnchorComponent::_onGeoreferenceChanged() {
  if (this->_actorToECEFIsValid) {
    this->SetActorToEarthCenteredEarthFixedMatrix(
        this->ActorToEarthCenteredEarthFixedMatrix);
  }
}
