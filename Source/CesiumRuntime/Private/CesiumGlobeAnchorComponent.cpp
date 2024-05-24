// Copyright 2020-2024 CesiumGS, Inc. and Contributors

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
// ## OriginLocation Changed
//
// * Handled by Unreal's normal `ApplyWorldOffset` mechanism.

namespace {

CesiumGeospatial::GlobeAnchor
createNativeGlobeAnchor(const FMatrix& actorToECEF) {
  return CesiumGeospatial::GlobeAnchor(VecMath::createMatrix4D(actorToECEF));
}

} // namespace

TSoftObjectPtr<ACesiumGeoreference>
UCesiumGlobeAnchorComponent::GetGeoreference() const {
  return this->Georeference;
}

void UCesiumGlobeAnchorComponent::SetGeoreference(
    TSoftObjectPtr<ACesiumGeoreference> NewGeoreference) {
  ACesiumGeoreference* pOriginal = this->ResolvedGeoreference;

  if (IsValid(pOriginal)) {
    pOriginal->OnGeoreferenceUpdated.RemoveAll(this);
  }

  this->ResolvedGeoreference = nullptr;
  this->Georeference = NewGeoreference;

  // If this component is currently registered, we need to re-resolve the
  // georeference. If it's not, this will happen when it becomes registered.
  if (this->IsRegistered()) {
    this->ResolveGeoreference();
  }
}

ACesiumGeoreference*
UCesiumGlobeAnchorComponent::GetResolvedGeoreference() const {
  return this->ResolvedGeoreference;
}

FVector
UCesiumGlobeAnchorComponent::GetEarthCenteredEarthFixedPosition() const {
  if (!this->_actorToECEFIsValid) {
    // Only log a warning if we're actually in a world. Otherwise we'll spam the
    // log when editing a CDO.
    if (this->GetWorld()) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "CesiumGlobeAnchorComponent %s globe position is invalid because the component is not yet registered."),
          *this->GetName());
    }
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

  UCesiumEllipsoid* ellipsoid = this->Georeference->GetEllipsoid();

  // Compute the current local up axis of the actor (the +Z axis) in ECEF
  FVector up = this->ActorToEarthCenteredEarthFixedMatrix.GetUnitAxis(EAxis::Z);

  // Compute the surface normal of the ellipsoid
  FVector ellipsoidNormal = ellipsoid->GeodeticSurfaceNormal(
      this->GetEarthCenteredEarthFixedPosition());

  // Find the shortest rotation to align local up with the ellipsoid normal
  FMatrix alignmentRotation =
      FQuat::FindBetween(up, ellipsoidNormal).ToMatrix();

  // Apply the new rotation to the Actor->ECEF transform.
  // Note that FMatrix multiplication order is opposite glm::dmat4
  // multiplication order!
  FMatrix newActorToECEF =
      this->ActorToEarthCenteredEarthFixedMatrix * alignmentRotation;

  // We don't want to rotate the origin, though, so re-set it.
  newActorToECEF.SetOrigin(
      this->ActorToEarthCenteredEarthFixedMatrix.GetOrigin());

  this->_updateFromNativeGlobeAnchor(createNativeGlobeAnchor(newActorToECEF));

#if WITH_EDITOR
  // In the Editor, mark this component modified so Undo works properly.
  this->Modify();
#endif
}

void UCesiumGlobeAnchorComponent::SnapToEastSouthUp() {
  this->SetEastSouthUpRotation(FQuat::Identity);

#if WITH_EDITOR
  // In the Editor, mark this component modified so Undo works properly.
  this->Modify();
#endif
}

void UCesiumGlobeAnchorComponent::Sync() {
  // If we don't have a actor -> ECEF matrix yet, we must update from the
  // actor's root transform.
  bool updateFromTransform = !this->_actorToECEFIsValid;
  if (!updateFromTransform && this->_lastRelativeTransformIsValid) {
    // We may also need to update from the Transform if it has changed
    // since the last time we computed the local -> globe fixed matrix.
    updateFromTransform = !this->_lastRelativeTransform.Equals(
        this->_getCurrentRelativeTransform(),
        0.0);
  }

  if (updateFromTransform) {
    this->_setNewActorToECEFFromRelativeTransform();
  } else {
    this->SetActorToEarthCenteredEarthFixedMatrix(
        this->ActorToEarthCenteredEarthFixedMatrix);
  }
}

ACesiumGeoreference*
UCesiumGlobeAnchorComponent::ResolveGeoreference(bool bForceReresolve) {
  if (IsValid(this->ResolvedGeoreference) && !bForceReresolve) {
    return this->ResolvedGeoreference;
  }

  ACesiumGeoreference* Previous = this->ResolvedGeoreference;
  ACesiumGeoreference* Next =
      IsValid(this->Georeference.Get())
          ? this->ResolvedGeoreference = this->Georeference.Get()
          : ACesiumGeoreference::GetDefaultGeoreferenceForActor(
                this->GetOwner());

  if (Previous != Next) {
    if (IsValid(Previous)) {
      // If we previously had a valid georeference, first synchronize using the
      // old one so that the ECEF and Actor transforms are both up-to-date.
      this->Sync();

      Previous->OnGeoreferenceUpdated.RemoveAll(this);
    }

    this->ResolvedGeoreference = Next;

    if (this->ResolvedGeoreference) {
      this->ResolvedGeoreference->OnGeoreferenceUpdated.AddUniqueDynamic(
          this,
          &UCesiumGlobeAnchorComponent::_onGeoreferenceChanged);

      // Now synchronize based on the new georeference.
      this->Sync();
    }
  }

  return this->ResolvedGeoreference;
}

UCesiumEllipsoid* UCesiumGlobeAnchorComponent::GetEllipsoid() const {
  return this->GetGeoreference()->GetEllipsoidConst();
}

void UCesiumGlobeAnchorComponent::InvalidateResolvedGeoreference() {
  // This method is deprecated and no longer does anything.
}

FVector UCesiumGlobeAnchorComponent::GetLongitudeLatitudeHeight() const {
  return this->GetEllipsoid()->
      EllipsoidCenteredEllipsoidFixedToLongitudeLatitudeHeight(
          this->GetEarthCenteredEarthFixedPosition());
}

void UCesiumGlobeAnchorComponent::MoveToLongitudeLatitudeHeight(
    const FVector& TargetLongitudeLatitudeHeight) {
  this->MoveToEarthCenteredEarthFixedPosition(
      this->GetEllipsoid()->LongitudeLatitudeHeightToEllipsoidCenteredEllipsoidFixed(
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
  if (!this->_actorToECEFIsValid) {
    // Only log a warning if we're actually in a world. Otherwise we'll spam the
    // log when editing a CDO.
    if (this->GetWorld()) {
      UE_LOG(
          LogCesium,
          Error,
          TEXT(
              "Cannot get the rotation from CesiumGlobeAnchorComponent %s because the component is not yet registered or does not have a valid CesiumGeoreference."),
          *this->GetName());
    }
    return FQuat::Identity;
  }

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
  if (!this->_actorToECEFIsValid) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Cannot set the rotation on CesiumGlobeAnchorComponent %s because the component is not yet registered or does not have a valid CesiumGeoreference."),
        *this->GetName());
    return;
  }

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
  if (!this->_actorToECEFIsValid) {
    // Only log a warning if we're actually in a world. Otherwise we'll spam the
    // log when editing a CDO.
    if (this->GetWorld()) {
      UE_LOG(
          LogCesium,
          Error,
          TEXT(
              "Cannot get the rotation from CesiumGlobeAnchorComponent %s because the component is not yet registered or does not have a valid CesiumGeoreference."),
          *this->GetName());
    }
    return FQuat::Identity;
  }

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
  if (!this->_actorToECEFIsValid) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Cannot set the rotation on CesiumGlobeAnchorComponent %s because the component is not yet registered or does not have a valid CesiumGeoreference."),
        *this->GetName());
    return;
  }

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
  if (PropertyChangedEvent.Property) {
    FName propertyName = PropertyChangedEvent.Property->GetFName();

    if (propertyName ==
        GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorComponent, Georeference)) {
      this->SetGeoreference(this->Georeference);
    }
  }

  // Call the base class implementation last, because it will call OnRegister,
  // which will call Sync. So we need to apply the updated values first.
  Super::PostEditChangeProperty(PropertyChangedEvent);

  // Calling the Super implementation above shouldn't change the current
  // relative transform without also changing _lastRelativeTransform. Except it
  // can, because in some cases (e.g., on undo/redo), Unreal reruns the Actor's
  // construction scripts, which can cause the relative transform to be
  // recomputed from the world transform. That's a problem because later we'll
  // think the relative transform has changed and will recompute the globe
  // transform from it. So we set (again) the _lastRelativeTransform here.
  //
  // One possible danger is that the construction script _intentionally_ changes
  // the transform. But we can't reliably distinguish that case from a phantom
  // transform "change" caused by converting to a world transform and back. And
  // surely something dodgy would be happening if something _other_ than the
  // globe anchor were intentionally moving the Actor in the globe anchor's
  // PostEditChangeProperty, right?
  if (this->_lastRelativeTransformIsValid) {
    this->_lastRelativeTransform = this->_getCurrentRelativeTransform();
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

  this->ResolveGeoreference();
}

void UCesiumGlobeAnchorComponent::OnUnregister() {
  Super::OnUnregister();

  // Unsubscribe from the ResolvedGeoreference.
  if (IsValid(this->ResolvedGeoreference)) {
    this->ResolvedGeoreference->OnGeoreferenceUpdated.RemoveAll(this);
  }
  this->ResolvedGeoreference = nullptr;

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

CesiumGeospatial::GlobeAnchor
UCesiumGlobeAnchorComponent::_createNativeGlobeAnchor() const {
  return createNativeGlobeAnchor(this->ActorToEarthCenteredEarthFixedMatrix);
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

  this->_lastRelativeTransform = this->_getCurrentRelativeTransform();
  this->_lastRelativeTransformIsValid = true;
}

CesiumGeospatial::GlobeAnchor UCesiumGlobeAnchorComponent::
    _createOrUpdateNativeGlobeAnchorFromRelativeTransform(
        const FTransform& newRelativeTransform) {
  ACesiumGeoreference* pGeoreference = this->ResolvedGeoreference;
  assert(pGeoreference != nullptr);

  const CesiumGeospatial::LocalHorizontalCoordinateSystem& local =
      pGeoreference->GetCoordinateSystem();

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

  // Update the Unreal relative transform
  ACesiumGeoreference* pGeoreference = this->ResolveGeoreference();
  if (IsValid(pGeoreference)) {
    glm::dmat4 anchorToLocal = nativeAnchor.getAnchorToLocalTransform(
        pGeoreference->GetCoordinateSystem());

    this->_setCurrentRelativeTransform(
        FTransform(VecMath::createMatrix(anchorToLocal)));
  } else {
    this->_lastRelativeTransformIsValid = false;
  }
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
  ACesiumGeoreference* pGeoreference = this->ResolveGeoreference();
  if (!IsValid(pGeoreference)) {
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
