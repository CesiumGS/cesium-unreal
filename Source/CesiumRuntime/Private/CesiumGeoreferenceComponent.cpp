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
#include "UObject/NameTypes.h"
#include "VecMath.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <optional>

UCesiumGeoreferenceComponent::UCesiumGeoreferenceComponent()
    : _worldOriginLocation(0.0),
      _relativeLocation(0.0)
      // _actorToECEF is initialized from property 
      {
  this->bAutoActivate = true;
  this->bWantsInitializeComponent = true;

  PrimaryComponentTick.bCanEverTick = false;

  // TODO: check when exactly constructor is called. Is it possible that it is
  // only called for CDO and then all other load/save/replications happen from
  // serialize/deserialize?
}


void UCesiumGeoreferenceComponent::SnapLocalUpToEllipsoidNormal() {

  // local up in ECEF (the +Z axis)
  glm::dvec3 actorUpECEF = glm::normalize(this->_actorToECEF[2]);

  // the surface normal of the ellipsoid model of the globe at the ECEF location
  // of the actor
  if (!IsValid(this->Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreferenceComponent does not have a valid Georeference"));
    return;
  }
  const glm::dvec3 ecef = _computeEcef();
  glm::dvec3 ellipsoidNormal =
      this->Georeference->ComputeGeodeticSurfaceNormal(ecef);

  // the shortest rotation to align local up with the ellipsoid normal
  glm::dquat R = glm::rotation(actorUpECEF, ellipsoidNormal);

  // We only want to apply the rotation to the actor's orientation, not
  // translation.
  this->_actorToECEF[0] = R * this->_actorToECEF[0];
  this->_actorToECEF[1] = R * this->_actorToECEF[1];
  this->_actorToECEF[2] = R * this->_actorToECEF[2];

  this->_updateActorTransform();
}

void UCesiumGeoreferenceComponent::SnapToEastSouthUp() {

  if (!IsValid(this->Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreferenceComponent does not have a valid Georeference"));
    return;
  }

  const glm::dvec3 ecef = _computeEcef();
  glm::dmat4 ENUtoECEF = glm::dmat4(
      this->Georeference->ComputeEastNorthUpToEcef(ecef));
  ENUtoECEF[3] = glm::dvec4(ecef, 1.0);

  this->_actorToECEF = ENUtoECEF * CesiumTransforms::scaleToCesium *
                       CesiumTransforms::unrealToOrFromCesium;

  this->_updateActorTransform();
}


void UCesiumGeoreferenceComponent::MoveToLongitudeLatitudeHeight(
    const glm::dvec3& targetLongitudeLatitudeHeight,
    bool maintainRelativeOrientation) {

  if (!IsValid(this->Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreferenceComponent does not have a valid Georeference"));
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


void UCesiumGeoreferenceComponent::OnRegister() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called OnRegister on component %s"),
      *this->GetName());
  Super::OnRegister();

  AActor *owner = this->GetOwner();
  USceneComponent *ownerRoot = owner->GetRootComponent();
  ownerRoot->TransformUpdated.AddUObject(this, &UCesiumGeoreferenceComponent::HandleActorTransformUpdated);
  //_updateRelativeLocationFromActor();
}


void UCesiumGeoreferenceComponent::OnUnregister() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called OnUnregister on component %s"),
      *this->GetName());
  Super::OnUnregister();

  AActor *owner = this->GetOwner();
  USceneComponent *ownerRoot = owner->GetRootComponent();
  ownerRoot->TransformUpdated.RemoveAll(this);
}

void UCesiumGeoreferenceComponent::HandleActorTransformUpdated(USceneComponent* InRootComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport) 
{
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called HandleActorTransformUpdated on component %s"),
      *this->GetName());
  _updateRelativeLocationFromActor();
}

glm::dvec3 UCesiumGeoreferenceComponent::_getRelativeLocationFromActor()
{
  AActor *owner = this->GetOwner();
  if (!IsValid(owner)){
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("Owner is not valid for %s"),
        *this->GetName());
    return glm::dvec3();
  }
  USceneComponent *ownerRoot = owner->GetRootComponent();
  const FVector& relativeLocation = ownerRoot->GetComponentLocation();
  return VecMath::createVector3D(relativeLocation);
}

void UCesiumGeoreferenceComponent::_updateRelativeLocationFromActor()
{
  this->_relativeLocation = _getRelativeLocationFromActor();
  _updateActorTransform();
  _updateDisplayECEF();
  _updateDisplayLongitudeLatitudeHeight();

  //this->_updateActorToECEF();

  // TODO: add warning or fix unstable behavior when autosnapping a translation
  // in terms of the local axes
  if (this->_autoSnapToEastSouthUp) {
    //this->SnapToEastSouthUp();
  }
}

void UCesiumGeoreferenceComponent::_initGeoreference()
{
  if (!this->Georeference) {
    this->Georeference = ACesiumGeoreference::GetDefaultGeoreference(this);
  }
  if (this->Georeference) {
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("Attaching CesiumGeoreferenceComponent callback to Georeference %s"),
        *this->Georeference->GetFullName());
    this->Georeference->OnGeoreferenceUpdated.AddUniqueDynamic(
        this,
        &UCesiumGeoreferenceComponent::HandleGeoreferenceUpdated);
    HandleGeoreferenceUpdated();
  }
}

void UCesiumGeoreferenceComponent::ApplyWorldOffset(
    const FVector& InOffset,
    bool bWorldShift) {
  UActorComponent::ApplyWorldOffset(InOffset, bWorldShift);

  glm::dvec3 oldOrigin = _worldOriginLocation;
  this->_worldOriginLocation = oldOrigin - VecMath::createVector3D(InOffset);
  
  //if (this->FixTransformOnOriginRebase) // TODO GEOREF_REFACTORING Figure out how to handle this...
  {
    this->_updateActorTransform();
  }
}
/*
void UCesiumGeoreferenceComponent::OnUpdateTransform(
    EUpdateTransformFlags UpdateTransformFlags,
    ETeleportType Teleport) {
  USceneComponent::OnUpdateTransform(UpdateTransformFlags, Teleport);

    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("Called OnUpdateTransform on %s"),
        *this->GetFullName());


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
*/

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
  } else if (propertyName == GET_MEMBER_NAME_CHECKED(UCesiumGeoreferenceComponent, Georeference)) {
    if (IsValid(this->Georeference)) {
      this->Georeference->OnGeoreferenceUpdated.AddUniqueDynamic(
          this,
          &UCesiumGeoreferenceComponent::HandleGeoreferenceUpdated);
      HandleGeoreferenceUpdated();
    }
    return;
  }
}
#endif

void UCesiumGeoreferenceComponent::HandleGeoreferenceUpdated() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called HandleGeoreferenceUpdated for %s"),
      *this->GetName());
  
  if (!IsValid(this->Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "CesiumGeoreferenceComponent does not have a valid Georeference"));
    return;
  }

  const glm::dmat4 ecefToUnreal = 
    this->Georeference->GetEllipsoidCenteredToUnrealWorldTransform();  
  const glm::dvec3 ecef = glm::dvec3(ECEF_X, ECEF_Y, ECEF_Z);
  const glm::dvec3 absoluteLocation = ecefToUnreal * glm::dvec4(ecef, 1.0);
  _relativeLocation = absoluteLocation - _worldOriginLocation;

  //this->_updateActorToECEF();
  this->_updateActorTransform();
  

  this->_updateDisplayECEF();
  this->_updateDisplayLongitudeLatitudeHeight();

  _logState();
}

void UCesiumGeoreferenceComponent::SetAutoSnapToEastSouthUp(bool value) {
  this->_autoSnapToEastSouthUp = value;
  if (value) {
    this->SnapToEastSouthUp();
  }
}



// TODO GEOREF_REFACTORING Remove these overrides,
// and the "bWantsInitializeComponent=true" that
// is set in the constructor!
void UCesiumGeoreferenceComponent::InitializeComponent() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called InitializeComponent on component %s"),
      *this->GetName());
  Super::InitializeComponent();
}
void UCesiumGeoreferenceComponent::PostInitProperties() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called PostInitProperties on component %s"),
      *this->GetName());
  Super::PostInitProperties();
}



// TODO GEOREF_REFACTORING Assuming these are the right places for the "_init" calls
void UCesiumGeoreferenceComponent::OnComponentCreated() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called OnComponentCreated on component %s"),
      *this->GetName());
  Super::OnComponentCreated();

  //_initGeoreference();
  //_initWorldOriginLocation();
}
void UCesiumGeoreferenceComponent::PostLoad() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called PostLoad on component %s"),
      *this->GetName());
  Super::PostLoad();

  //_initGeoreference();
  //_initWorldOriginLocation();
}



/**
 *  PRIVATE HELPER FUNCTIONS
 */


void UCesiumGeoreferenceComponent::_initWorldOriginLocation() {
  const FIntVector& origin = this->GetWorld()->OriginLocation;
  this->_worldOriginLocation = VecMath::createVector3D(origin);
}

glm::dvec3 UCesiumGeoreferenceComponent::_getAbsoluteLocationFromActor() {
  AActor *owner = this->GetOwner();
  if (!IsValid(owner)){
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("Owner is not valid for %s"),
        *this->GetName());
    return glm::dvec3();
  }
  USceneComponent *ownerRoot = owner->GetRootComponent();
  const FVector& relativeLocation = ownerRoot->GetComponentLocation();
  const FIntVector& originLocation = this->GetWorld()->OriginLocation;
  glm::dvec3 absoluteLocation = VecMath::add3D(originLocation, relativeLocation);
  return absoluteLocation;
}
/*

void UCesiumGeoreferenceComponent::_updateRelativeLocation() {
  // Note: Since we have a presumably accurate _absoluteLocation, this will be
  // more accurate than querying the floating-point UE relative world location.
  // This means that although the rendering, physics, and anything else on the
  // UE side might be jittery, our internal representation of the location will
  // remain accurate.
  this->_relativeLocation =
      this->_absoluteLocation - this->_worldOriginLocation;
}
*/


void logVector(const std::string& name, const glm::dvec3& vector) {
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("%s: %16.6f %16.6f %16.6f"), *FString(name.c_str()), vector.x, vector.y, vector.z);
}

void UCesiumGeoreferenceComponent::_logState() {
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("State of %s"), *this->GetName());
    logVector("  worldOriginLocation", this->_worldOriginLocation);
    logVector("  relativeLocation   ", this->_relativeLocation);
    logVector("  absoluteLocation   ", (_relativeLocation + _worldOriginLocation));
}



void logMatrix(const std::string& name, const glm::dmat4& matrix) {
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("%s:"), *FString(name.c_str()));
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT(" %16.6f %16.6f %16.6f %16.6f"), matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0]);
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT(" %16.6f %16.6f %16.6f %16.6f"), matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1]);
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT(" %16.6f %16.6f %16.6f %16.6f"), matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2]);
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT(" %16.6f %16.6f %16.6f %16.6f"), matrix[0][3], matrix[1][3], matrix[2][3], matrix[3][3]);
}

/*
void UCesiumGeoreferenceComponent::_updateActorToECEF() {
  if (!IsValid(this->Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreferenceComponent does not have a valid Georeference"));
    return;
  }

  glm::dmat4 unrealWorldToEcef =
      this->Georeference->GetUnrealWorldToEllipsoidCenteredTransform();

  AActor *owner = this->GetOwner();
  USceneComponent *ownerRoot = owner->GetRootComponent();
  FMatrix actorToRelativeWorld =
      ownerRoot->GetComponentToWorld().ToMatrixWithScale();
  glm::dmat4 actorToAbsoluteWorld =
      VecMath::createMatrix4D(actorToRelativeWorld, 
        _worldOriginLocation + _relativeLocation);

  this->_actorToECEF = unrealWorldToEcef * actorToAbsoluteWorld;

  logMatrix("actorToAbsoluteWorld", actorToAbsoluteWorld);
  logMatrix("unrealWorldToEcef", unrealWorldToEcef);
  logMatrix("_actorToECEF", _actorToECEF);

  this->_updateDisplayECEF();
  this->_updateDisplayLongitudeLatitudeHeight();
}
*/


void UCesiumGeoreferenceComponent::_updateActorTransform()
{
  if (!this->GetWorld()) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreferenceComponent is not spawned in world"));
    return;
  }

  AActor *owner = this->GetOwner();
  USceneComponent *ownerRoot = owner->GetRootComponent();

  FMatrix actorToRelativeWorldLow =
      ownerRoot->GetComponentToWorld().ToMatrixWithScale();
  glm::dmat4 actorRelativeWorldHigh = VecMath::createMatrix4D(
    actorToRelativeWorldLow, _relativeLocation);
  FMatrix actorToRelativeWorld = 
    VecMath::createMatrix(actorRelativeWorldHigh);

  ownerRoot->SetWorldTransform(
      FTransform(actorToRelativeWorld),
      false,
      nullptr,
      TeleportWhenUpdatingTransform ? ETeleportType::TeleportPhysics
                                    : ETeleportType::None);
}


void UCesiumGeoreferenceComponent::_setECEF(
    const glm::dvec3& targetEcef,
    bool maintainRelativeOrientation) {

  logVector("_setECEF targetEcef ", targetEcef);

  if (!IsValid(this->Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "CesiumGeoreferenceComponent does not have a valid Georeference"));
    return;
  }

  /*
  if (maintainRelativeOrientation) {

    // TODO GEOREF_REFACTORING Figure out what that was supposed to accomplish...

    // Note: this probably degenerates when starting at or moving to either of
    // the poles

    const glm::dvec3 ecef = _computeEcef();
    glm::dmat4 startEcefToEnuBase = glm::dmat4(this->Georeference->ComputeEastNorthUpToEcef(ecef));
    startEcefToEnuBase[3] = glm::dvec4(ecef, 1.0);
    glm::dmat4 startEcefToEnu = glm::affineInverse(startEcefToEnuBase);

    glm::dmat4 endEnuToEcef = glm::dmat4(this->Georeference->ComputeEastNorthUpToEcef(targetEcef));
    endEnuToEcef[3] = glm::dvec4(targetEcef, 1.0);

    this->_actorToECEF = endEnuToEcef * startEcefToEnu * this->_actorToECEF;

    return;    
  }
  */

  // Not maintaining relative orientation:
  const glm::dmat4& ecefToUnreal = this->Georeference->GetEllipsoidCenteredToUnrealWorldTransform();
  const glm::dvec3 absoluteLocation = ecefToUnreal * glm::dvec4(targetEcef, 1.0);
  this->_relativeLocation = absoluteLocation - _worldOriginLocation;

  _updateActorTransform();

  // If the transform needs to be snapped to the tangent plane, do it here.
  if (this->_autoSnapToEastSouthUp) {
    //this->SnapToEastSouthUp();
  }

  // Update component properties
  this->_updateDisplayECEF();
  this->_updateDisplayLongitudeLatitudeHeight();
}


void UCesiumGeoreferenceComponent::_updateDisplayLongitudeLatitudeHeight() {

  if (!IsValid(this->Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("CesiumGeoreferenceComponent does not have a valid Georeference"));
    return;
  }
  const glm::dvec3 ecef = _computeEcef();
  const glm::dvec3 cartographic =
      this->Georeference->TransformEcefToLongitudeLatitudeHeight(ecef);
  this->Longitude = cartographic.x;
  this->Latitude = cartographic.y;
  this->Height = cartographic.z;

  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called _updateDisplayLongitudeLatitudeHeight with height %f on component %s"),
      this->Height, *this->GetName());


}

glm::dvec3 UCesiumGeoreferenceComponent::_computeEcef() {
  if (!IsValid(this->Georeference)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "CesiumGeoreferenceComponent does not have a valid Georeference"));
    return glm::dvec3(0.0);
  }
  const glm::dmat4& unrealToEcef = this->Georeference->GetUnrealWorldToEllipsoidCenteredTransform();
  const glm::dvec3 absoluteLocation = (_relativeLocation + _worldOriginLocation);
  const glm::dvec3 ecef = unrealToEcef * glm::dvec4(absoluteLocation, 1.0);
  return ecef;
}

void UCesiumGeoreferenceComponent::_updateDisplayECEF() {
  const glm::dvec3 ecef = _computeEcef();
  this->ECEF_X = ecef.x;
  this->ECEF_Y = ecef.y;
  this->ECEF_Z = ecef.z;
}
