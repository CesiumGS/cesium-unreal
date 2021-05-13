// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGeoreferenceComponent.h"
#include "CesiumGeospatial/Cartographic.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumGeospatial/Transforms.h"
#include "CesiumTransforms.h"
#include "CesiumUtility/Math.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "UObject/NameTypes.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/quaternion.hpp>
#include <optional>

UCesiumGeoreferenceComponent::UCesiumGeoreferenceComponent()
    : _worldOriginLocation(0.0),
      _absoluteLocation(0.0),
      _relativeLocation(0.0),
      //_actorToECEF(),
      _actorToUnrealRelativeWorld(),
      _ownerRoot(nullptr),
      _georeferenced(false),
      _ignoreOnUpdateTransform(false),
      //_autoSnapToEastSouthUp(false),
      _dirty(false) {
  this->bAutoActivate = true;
  this->bWantsOnUpdateTransform = true;
  PrimaryComponentTick.bCanEverTick = false;

  // TODO: check when exactly constructor is called. Is it possible that it is
  // only called for CDO and then all other load/save/replications happen from
  // serialize/deserialize?

  // set a delegate callback when the root component for the actor is set
  this->IsRootComponentChanged.AddDynamic(
      this,
      &UCesiumGeoreferenceComponent::OnRootComponentChanged);
}

void UCesiumGeoreferenceComponent::SnapLocalUpToEllipsoidNormal() {
  // local up in ECEF (the +Z axis)
  glm::dvec3 actorUpECEF = glm::normalize(this->_actorToECEF[2]);

  // the surface normal of the ellipsoid model of the globe at the ECEF location
  // of the actor
  glm::dvec3 ellipsoidNormal =
      CesiumGeospatial::Ellipsoid::WGS84.geodeticSurfaceNormal(
          this->_actorToECEF[3]);

  // the shortest rotation to align local up with the ellipsoid normal
  glm::dquat R = glm::rotation(actorUpECEF, ellipsoidNormal);

  // We only want to apply the rotation to the actor's orientation, not
  // translation.
  this->_actorToECEF[0] = R * this->_actorToECEF[0];
  this->_actorToECEF[1] = R * this->_actorToECEF[1];
  this->_actorToECEF[2] = R * this->_actorToECEF[2];

  this->_updateActorToUnrealRelativeWorldTransform();
  this->_setTransform(this->_actorToUnrealRelativeWorld);
}

void UCesiumGeoreferenceComponent::SnapToEastSouthUp() {
  glm::dmat4 ENUtoECEF = CesiumGeospatial::Transforms::eastNorthUpToFixedFrame(
      this->_actorToECEF[3]);
  this->_actorToECEF = ENUtoECEF * CesiumTransforms::scaleToCesium *
                       CesiumTransforms::unrealToOrFromCesium;
  ;
  this->_updateActorToUnrealRelativeWorldTransform();
  this->_setTransform(this->_actorToUnrealRelativeWorld);
}

void UCesiumGeoreferenceComponent::MoveToLongitudeLatitudeHeight(
    const glm::dvec3& targetLongitudeLatitudeHeight,
    bool maintainRelativeOrientation) {
  glm::dvec3 ecef = CesiumGeospatial::Ellipsoid::WGS84.cartographicToCartesian(
      CesiumGeospatial::Cartographic::fromDegrees(
          targetLongitudeLatitudeHeight.x,
          targetLongitudeLatitudeHeight.y,
          targetLongitudeLatitudeHeight.z));

  this->_setECEF(ecef, maintainRelativeOrientation);
}

void UCesiumGeoreferenceComponent::InaccurateMoveToLongitudeLatitudeHeight(
    const FVector& targetLongitudeLatitudeHeight,
    bool maintainRelativeOrientation) {
  this->MoveToLongitudeLatitudeHeight(
      glm::dvec3(
          targetLongitudeLatitudeHeight.X,
          targetLongitudeLatitudeHeight.Y,
          targetLongitudeLatitudeHeight.Z),
      maintainRelativeOrientation);
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
      glm::dvec3(targetEcef.X, targetEcef.Y, targetEcef.Z),
      maintainRelativeOrientation);
}

// TODO: is this the best place to attach to the root component of the owner
// actor?
void UCesiumGeoreferenceComponent::OnRegister() {
  Super::OnRegister();
  this->_initRootComponent();
}

// TODO: figure out what these delegate parameters actually represent, currently
// I'm only guessing based on the types
void UCesiumGeoreferenceComponent::OnRootComponentChanged(
    USceneComponent* UpdatedComponent,
    bool bIsRootComponent) {
  this->_initRootComponent();
}

void UCesiumGeoreferenceComponent::ApplyWorldOffset(
    const FVector& InOffset,
    bool bWorldShift) {
  // USceneComponent::ApplyWorldOffset will call OnUpdateTransform, we want to
  // ignore it since we don't have to recompute everything on origin rebase.
  this->_ignoreOnUpdateTransform = true;
  USceneComponent::ApplyWorldOffset(InOffset, bWorldShift);

  const FIntVector& oldOrigin = this->GetWorld()->OriginLocation;
  this->_worldOriginLocation = glm::dvec3(
      static_cast<double>(oldOrigin.X) - static_cast<double>(InOffset.X),
      static_cast<double>(oldOrigin.Y) - static_cast<double>(InOffset.Y),
      static_cast<double>(oldOrigin.Z) - static_cast<double>(InOffset.Z));

  // Do _not_ call _updateAbsoluteLocation. The absolute position doesn't change
  // with an origin rebase, and we'll lose precision if we update the absolute
  // location here.

  this->_updateRelativeLocation();
  this->_updateActorToUnrealRelativeWorldTransform();
  if (this->FixTransformOnOriginRebase) {
    this->_setTransform(this->_actorToUnrealRelativeWorld);
  }
}

void UCesiumGeoreferenceComponent::OnUpdateTransform(
    EUpdateTransformFlags UpdateTransformFlags,
    ETeleportType Teleport) {
  USceneComponent::OnUpdateTransform(UpdateTransformFlags, Teleport);

  // if we generated this transform call internally, we should ignore it
  if (this->_ignoreOnUpdateTransform) {
    this->_ignoreOnUpdateTransform = false;
    return;
  }

  this->_updateAbsoluteLocation();
  this->_updateRelativeLocation();
  this->_updateActorToECEF();
  this->_updateActorToUnrealRelativeWorldTransform();

  // TODO: add warning or fix unstable behavior when autosnapping a translation
  // in terms of the local axes
  if (this->_autoSnapToEastSouthUp) {
    this->SnapToEastSouthUp();
  }
}

void UCesiumGeoreferenceComponent::BeginPlay() { Super::BeginPlay(); }

bool UCesiumGeoreferenceComponent::MoveComponentImpl(
    const FVector& Delta,
    const FQuat& NewRotation,
    bool bSweep,
    FHitResult* OutHit,
    EMoveComponentFlags MoveFlags,
    ETeleportType Teleport) {
  if (this->_ownerRoot != this) {
    return false;
  }
  return USceneComponent::MoveComponentImpl(
      Delta,
      NewRotation,
      bSweep,
      OutHit,
      MoveFlags,
      Teleport);
}

#if WITH_EDITOR
void UCesiumGeoreferenceComponent::PostEditChangeProperty(
    FPropertyChangedEvent& event) {
  Super::PostEditChangeProperty(event);

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
#endif

void UCesiumGeoreferenceComponent::OnComponentDestroyed(
    bool bDestroyingHierarchy) {
  Super::OnComponentDestroyed(bDestroyingHierarchy);
}

bool UCesiumGeoreferenceComponent::IsBoundingVolumeReady() const {
  return false;
}

std::optional<Cesium3DTiles::BoundingVolume>
UCesiumGeoreferenceComponent::GetBoundingVolume() const {
  return std::nullopt;
}

void UCesiumGeoreferenceComponent::NotifyGeoreferenceUpdated() {
  this->_updateActorToUnrealRelativeWorldTransform();
  this->_setTransform(this->_actorToUnrealRelativeWorld);
}

void UCesiumGeoreferenceComponent::SetAutoSnapToEastSouthUp(bool value) {
  this->_autoSnapToEastSouthUp = value;
  if (value) {
    this->SnapToEastSouthUp();
  }
}

/**
 *  PRIVATE HELPER FUNCTIONS
 */

void UCesiumGeoreferenceComponent::_initRootComponent() {
  AActor* owner = this->GetOwner();
  this->_ownerRoot = owner->GetRootComponent();

  if (!this->_ownerRoot || !this->GetWorld()) {
    return;
  }

  // if this is not the root component, we need to attach to the root component
  // and control it
  if (this->_ownerRoot != this) {
    this->AttachToComponent(
        this->_ownerRoot,
        FAttachmentTransformRules::SnapToTargetIncludingScale);
  }

  this->_initWorldOriginLocation();
  this->_updateAbsoluteLocation();
  this->_updateRelativeLocation();
  this->_initGeoreference();
}

void UCesiumGeoreferenceComponent::_initWorldOriginLocation() {
  const FIntVector& origin = this->GetWorld()->OriginLocation;
  this->_worldOriginLocation = glm::dvec3(
      static_cast<double>(origin.X),
      static_cast<double>(origin.Y),
      static_cast<double>(origin.Z));
}

void UCesiumGeoreferenceComponent::_updateAbsoluteLocation() {
  const FVector& relativeLocation = this->_ownerRoot->GetComponentLocation();
  const FIntVector& originLocation = this->GetWorld()->OriginLocation;
  this->_absoluteLocation = glm::dvec3(
      static_cast<double>(originLocation.X) +
          static_cast<double>(relativeLocation.X),
      static_cast<double>(originLocation.Y) +
          static_cast<double>(relativeLocation.Y),
      static_cast<double>(originLocation.Z) +
          static_cast<double>(relativeLocation.Z));
}

void UCesiumGeoreferenceComponent::_updateRelativeLocation() {
  // Note: Since we have a presumably accurate _absoluteLocation, this will be
  // more accurate than querying the floating-point UE relative world location.
  // This means that although the rendering, physics, and anything else on the
  // UE side might be jittery, our internal representation of the location will
  // remain accurate.
  this->_relativeLocation =
      this->_absoluteLocation - this->_worldOriginLocation;
}

void UCesiumGeoreferenceComponent::_initGeoreference() {
  // if the georeference already exists, so does _actorToECEF so we don't have
  // to update it
  if (this->Georeference) {
    if (!this->_georeferenced) {
      this->Georeference->AddGeoreferencedObject(this);
      this->_georeferenced = true;
      return;
    }

    this->_updateActorToUnrealRelativeWorldTransform();
    this->_setTransform(this->_actorToUnrealRelativeWorld);
    return;
  }

  this->Georeference =
      ACesiumGeoreference::GetDefaultForActor(this->GetOwner());
  if (this->Georeference) {
    this->_updateActorToECEF();
    this->Georeference->AddGeoreferencedObject(this);
    this->_georeferenced = true;
  }

  // Note: when a georeferenced object is added, NotifyGeoreferenceUpdated will
  // automatically be called
}

// this is what georeferences the actor
void UCesiumGeoreferenceComponent::_updateActorToECEF() {
  if (!this->Georeference) {
    return;
  }

  const glm::dmat4& unrealWorldToEcef =
      this->Georeference->GetUnrealWorldToEllipsoidCenteredTransform();

  FMatrix actorToRelativeWorld =
      this->_ownerRoot->GetComponentToWorld().ToMatrixWithScale();
  glm::dmat4 actorToAbsoluteWorld(
      glm::dvec4(
          actorToRelativeWorld.M[0][0],
          actorToRelativeWorld.M[0][1],
          actorToRelativeWorld.M[0][2],
          actorToRelativeWorld.M[0][3]),
      glm::dvec4(
          actorToRelativeWorld.M[1][0],
          actorToRelativeWorld.M[1][1],
          actorToRelativeWorld.M[1][2],
          actorToRelativeWorld.M[1][3]),
      glm::dvec4(
          actorToRelativeWorld.M[2][0],
          actorToRelativeWorld.M[2][1],
          actorToRelativeWorld.M[2][2],
          actorToRelativeWorld.M[2][3]),
      glm::dvec4(this->_absoluteLocation, 1.0));

  this->_actorToECEF = unrealWorldToEcef * actorToAbsoluteWorld;
  this->_updateDisplayECEF();
  this->_updateDisplayLongitudeLatitudeHeight();
}

void UCesiumGeoreferenceComponent::
    _updateActorToUnrealRelativeWorldTransform() {
  if (!this->Georeference) {
    return;
  }
  const glm::dmat4& ecefToUnrealWorld =
      this->Georeference->GetEllipsoidCenteredToUnrealWorldTransform();
  glm::dmat4 absoluteToRelativeWorld(
      glm::dvec4(1.0, 0.0, 0.0, 0.0),
      glm::dvec4(0.0, 1.0, 0.0, 0.0),
      glm::dvec4(0.0, 0.0, 1.0, 0.0),
      glm::dvec4(-this->_worldOriginLocation, 1.0));

  this->_actorToUnrealRelativeWorld =
      absoluteToRelativeWorld * ecefToUnrealWorld * this->_actorToECEF;
}

void UCesiumGeoreferenceComponent::_setTransform(const glm::dmat4& transform) {
  if (!this->GetWorld()) {
    return;
  }

  // We are about to get an OnUpdateTransform callback for this, so we
  // preemptively mark down to ignore it.
  _ignoreOnUpdateTransform = true;

  this->_ownerRoot->SetWorldTransform(FTransform(FMatrix(
      FVector(transform[0].x, transform[0].y, transform[0].z),
      FVector(transform[1].x, transform[1].y, transform[1].z),
      FVector(transform[2].x, transform[2].y, transform[2].z),
      FVector(transform[3].x, transform[3].y, transform[3].z))));

  // TODO: try direct setting of transformation, may work for static objects on
  // origin rebase
  /*
  this->_ownerRoot->SetRelativeLocation_Direct(
      FVector(transform[3].x, transform[3].y, transform[3].z));

  this->_ownerRoot->SetRelativeRotation_Direct(FMatrix(
    FVector(transform[0].x, transform[0].y, transform[0].z),
    FVector(transform[1].x, transform[1].y, transform[1].z),
    FVector(transform[2].x, transform[2].y, transform[2].z),
    FVector(0.0, 0.0, 0.0)
  ).Rotator());

  this->_ownerRoot->SetComponentToWorld(this->_ownerRoot->GetRelativeTransform());
  */
}

void UCesiumGeoreferenceComponent::_setECEF(
    const glm::dvec3& targetEcef,
    bool maintainRelativeOrientation) {
  if (!maintainRelativeOrientation) {
    this->_actorToECEF[3] = glm::dvec4(targetEcef, 1.0);
  } else {
    // Note: this probably degenerates when starting at or moving to either of
    // the poles
    glm::dmat4 startEcefToEnu = glm::affineInverse(
        CesiumGeospatial::Transforms::eastNorthUpToFixedFrame(
            this->_actorToECEF[3]));
    glm::dmat4 endEnuToEcef =
        CesiumGeospatial::Transforms::eastNorthUpToFixedFrame(targetEcef);
    this->_actorToECEF = endEnuToEcef * startEcefToEnu * this->_actorToECEF;
  }

  this->_updateActorToUnrealRelativeWorldTransform();
  this->_setTransform(this->_actorToUnrealRelativeWorld);

  // In this case the ground truth is the newly updated _actorToECEF
  // transformation, so it will be more accurate to compute the new Unreal
  // locations this way (as opposed to _updateAbsoluteLocation /
  // _updateRelativeLocation).
  this->_relativeLocation = this->_actorToUnrealRelativeWorld[3];
  this->_absoluteLocation =
      this->_relativeLocation + this->_worldOriginLocation;

  // If the transform needs to be snapped to the tangent plane, do it here.
  if (this->_autoSnapToEastSouthUp) {
    this->SnapToEastSouthUp();
  }

  // Update component properties
  this->_updateDisplayECEF();
  this->_updateDisplayLongitudeLatitudeHeight();
}

void UCesiumGeoreferenceComponent::_updateDisplayLongitudeLatitudeHeight() {
  std::optional<CesiumGeospatial::Cartographic> cartographic =
      CesiumGeospatial::Ellipsoid::WGS84.cartesianToCartographic(
          this->_actorToECEF[3]);

  if (!cartographic) {
    // only happens when actor is too close to the center of the Earth
    return;
  }

  this->_dirty = true;

  this->Longitude =
      CesiumUtility::Math::radiansToDegrees((*cartographic).longitude);
  this->Latitude =
      CesiumUtility::Math::radiansToDegrees((*cartographic).latitude);
  this->Height = (*cartographic).height;
}

void UCesiumGeoreferenceComponent::_updateDisplayECEF() {
  this->_dirty = true;

  this->ECEF_X = this->_actorToECEF[3].x;
  this->ECEF_Y = this->_actorToECEF[3].y;
  this->ECEF_Z = this->_actorToECEF[3].z;
}
