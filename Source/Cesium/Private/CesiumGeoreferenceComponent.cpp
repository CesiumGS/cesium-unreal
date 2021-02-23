#include "CesiumGeoreferenceComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "UObject/NameTypes.h"
#include "CesiumTransforms.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumGeospatial/Cartographic.h"
#include "CesiumGeospatial/Transforms.h"
#include "CesiumUtility/Math.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <optional>

UCesiumGeoreferenceComponent::UCesiumGeoreferenceComponent() :
	_worldOriginLocation(0.0),
	_absoluteLocation(0.0),
	_relativeLocation(0.0),
	_actorToECEF(),
	_actorToUnrealRelativeWorld(),
	_ownerRoot(nullptr),
	_ignoreOnUpdateTransform(false)
{
	this->bAutoActivate = true;
	this->bWantsOnUpdateTransform = true;
	PrimaryComponentTick.bCanEverTick = false;

	// set a delegate callback when the root component for the actor is set 
	this->IsRootComponentChanged.AddDynamic(this, &UCesiumGeoreferenceComponent::OnRootComponentChanged);
}

void UCesiumGeoreferenceComponent::SnapLocalUpToEllipsoidNormal() {
	// local up in ECEF (the +Z axis)
	glm::dvec3 actorUpECEF = glm::normalize(this->_actorToECEF[2]);

	// the surface normal of the ellipsoid model of the globe at the ECEF location of the actor
	glm::dvec3 ellipsoidNormal = CesiumGeospatial::Ellipsoid::WGS84.geodeticSurfaceNormal(this->_actorToECEF[3]);
	
	// cosine of the angle between the actor's up direction and the ellipsoid normal
	double cos = glm::dot(actorUpECEF, ellipsoidNormal);

	// TODO: is this a reasonable epsilon in this case?
	if (cos < CesiumUtility::Math::EPSILON7 - 1.0) {
		// The actor's current up direction is completely upside down with respect to the ellipsoid normal.
		
		// We want to do a 180 degree rotation around X. We can do this by flipping the Y and Z axes 
		this->_actorToECEF[1] *= -1.0;
		this->_actorToECEF[2] *= -1.0;

	} else {
		// the axis of the shortest available rotation with a magnitude that is sine of the angle
		glm::dvec3 sin_axis = glm::cross(ellipsoidNormal, actorUpECEF);
		
		// We construct a rotation matrix using Rodrigues' rotation formula for rotating by theta around an axis. 		
		
		// K is the cross product matrix of the axis, i.e. K v = axis x v, where v is any vector. Here we have a factor
		// of sine theta that we let through as well since it will simplify the calcuations in Rodrigues` formula.
		glm::dmat3 sin_K(
			0.0, -sin_axis.z, sin_axis.y,
			sin_axis.z, 0.0, -sin_axis.x,
			-sin_axis.y, sin_axis.x, 0.0 
		);
		// Rodrigues' rotation formula
		glm::dmat4 R = glm::dmat3(1.0) + sin_K + sin_K * sin_K / (1.0 + cos);

		// We only want to apply the rotation to the actor's orientation, not translation.
		this->_actorToECEF[0] = R * this->_actorToECEF[0];
		this->_actorToECEF[1] = R * this->_actorToECEF[1];
		this->_actorToECEF[2] = R * this->_actorToECEF[2];
	}

	this->_updateActorToUnrealRelativeWorldTransform();
	this->_setTransform(this->_actorToUnrealRelativeWorld);
}

void UCesiumGeoreferenceComponent::SnapToEastSouthUpTangentPlane() {
	glm::dmat4 ENUtoECEF = CesiumGeospatial::Transforms::eastNorthUpToFixedFrame(this->_actorToECEF[3]);
	
	this->_actorToECEF = ENUtoECEF * CesiumTransforms::scaleToCesium * CesiumTransforms::unrealToOrFromCesium;;

	this->_updateActorToUnrealRelativeWorldTransform();
	this->_setTransform(this->_actorToUnrealRelativeWorld);
}

void UCesiumGeoreferenceComponent::MoveToLongLatHeight(double longitude, double latitude, double height) {
	glm::dvec3 ecef = CesiumGeospatial::Ellipsoid::WGS84.cartographicToCartesian(
		CesiumGeospatial::Cartographic::fromDegrees(longitude, latitude, height)
	);
	this->MoveToECEF(ecef.x, ecef.y, ecef.z);
}

void UCesiumGeoreferenceComponent::InaccurateMoveToLongLatHeight(float longitude, float latitude, float height) {
	this->MoveToLongLatHeight(longitude, latitude, height);
}

void UCesiumGeoreferenceComponent::MoveToECEF(double ecef_x, double ecef_y, double ecef_z) {
	this->_actorToECEF[3] = glm::vec4(ecef_x, ecef_y, ecef_z, 1.0);
	this->_updateLongLatHeight();

	this->_updateActorToUnrealRelativeWorldTransform();
	this->_setTransform(this->_actorToUnrealRelativeWorld);

	// In this case the ground truth is the newly updated _actorToECEF transformation, so it will be more accurate to
	// compute the new Unreal locations this way (as opposed to _updateAbsoluteLocation / _updateRelativeLocation).
	this->_relativeLocation = this->_actorToUnrealRelativeWorld[3];
	this->_absoluteLocation = this->_relativeLocation + this->_worldOriginLocation;

	// If the transform needs to be snapped to the tangent plane, do it here.
	if (this->_autoSnapToEastSouthUp) {
		this->SnapToEastSouthUpTangentPlane();
	}
}

void UCesiumGeoreferenceComponent::InaccurateMoveToECEF(float ecef_x, float ecef_y, float ecef_z) {
	this->MoveToECEF(ecef_x, ecef_y, ecef_z);
}

// TODO: is this the best place to attach to the root component of the owner actor?
void UCesiumGeoreferenceComponent::OnRegister() {
	Super::OnRegister();
	this->_initRootComponent();
}

// TODO: figure out what these delegate parameters actually represent, currently I'm only guessing based on the types
void UCesiumGeoreferenceComponent::OnRootComponentChanged(USceneComponent* /*newRoot*/, bool /*addedOrRemoved*/) {
	this->_initRootComponent();
}

void UCesiumGeoreferenceComponent::ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) {
	USceneComponent::ApplyWorldOffset(InOffset, bWorldShift);

	const FIntVector& oldOrigin = this->GetWorld()->OriginLocation;
	this->_worldOriginLocation = glm::dvec3(
		static_cast<double>(oldOrigin.X) - static_cast<double>(InOffset.X),
		static_cast<double>(oldOrigin.Y) - static_cast<double>(InOffset.Y),
		static_cast<double>(oldOrigin.Z) - static_cast<double>(InOffset.Z)
	);

	// Do _not_ call _updateAbsoluteLocation. The absolute position doesn't change with
	// an origin rebase, and we'll lose precision if we update the absolute location here.

	this->_updateRelativeLocation();
	this->_updateActorToUnrealRelativeWorldTransform();
	this->_setTransform(this->_actorToUnrealRelativeWorld);
}

void UCesiumGeoreferenceComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport) {
	USceneComponent::OnUpdateTransform(UpdateTransformFlags, Teleport);
	
	// if we generated this transform call internally, we should ignore it
	if (_ignoreOnUpdateTransform) {
		_ignoreOnUpdateTransform = false;
		return;
	}

	this->_updateAbsoluteLocation();
	this->_updateRelativeLocation();
	this->_updateActorToECEF();
	this->_updateActorToUnrealRelativeWorldTransform();

	// if the transform update originates from unreal, snap the orientation to the tangent plane here if needed
	if (this->_autoSnapToEastSouthUp) {
		this->SnapToEastSouthUpTangentPlane();
	}
}

void UCesiumGeoreferenceComponent::BeginPlay() {
	Super::BeginPlay();
}


#if WITH_EDITOR
void UCesiumGeoreferenceComponent::PostEditChangeProperty(FPropertyChangedEvent& event) {
	Super::PostEditChangeProperty(event);

	if (!event.Property) {
		return;
	}

	FName propertyName = event.Property->GetFName();

	if (
		propertyName == GET_MEMBER_NAME_CHECKED(UCesiumGeoreferenceComponent, TargetLongitude) ||
		propertyName == GET_MEMBER_NAME_CHECKED(UCesiumGeoreferenceComponent, TargetLatitude) ||
		propertyName == GET_MEMBER_NAME_CHECKED(UCesiumGeoreferenceComponent, TargetHeight)
	) {
		this->MoveToLongLatHeight(TargetLongitude, TargetLatitude, TargetHeight);
		return;
	} else if (
		propertyName == GET_MEMBER_NAME_CHECKED(UCesiumGeoreferenceComponent, TargetECEF_X) ||
		propertyName == GET_MEMBER_NAME_CHECKED(UCesiumGeoreferenceComponent, TargetECEF_Y) ||
		propertyName == GET_MEMBER_NAME_CHECKED(UCesiumGeoreferenceComponent, TargetECEF_Z)
	) {
		this->MoveToECEF(TargetECEF_X, TargetECEF_Y, TargetECEF_Z);
		return;
	}
}
#endif


void UCesiumGeoreferenceComponent::OnComponentDestroyed(bool bDestroyingHierarchy) {
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

bool UCesiumGeoreferenceComponent::IsBoundingVolumeReady() const {
	return false;
}

std::optional<Cesium3DTiles::BoundingVolume> UCesiumGeoreferenceComponent::GetBoundingVolume() const {
	return std::nullopt;
}

void UCesiumGeoreferenceComponent::UpdateGeoreferenceTransform(const glm::dmat4& ellipsoidCenteredToGeoreferencedTransform) {
	this->_updateActorToUnrealRelativeWorldTransform(ellipsoidCenteredToGeoreferencedTransform);
	this->_setTransform(this->_actorToUnrealRelativeWorld);
}

void UCesiumGeoreferenceComponent::SetAutoSnapToEastSouthUp(bool value) {
	this->_autoSnapToEastSouthUp = value;
	if (value) {
		this->SnapToEastSouthUpTangentPlane();
	}
}


/**
 *  PRIVATE HELPER FUNCTIONS
 */

void UCesiumGeoreferenceComponent::_initRootComponent() {
	AActor* owner = this->GetOwner(); 
	this->_ownerRoot = owner->GetRootComponent();

	if (!this->_ownerRoot) {
		return;
	}

	// if this is not the root component, we need to attach to the root component and control it
	if (this->_ownerRoot != this) {
		this->AttachToComponent(this->_ownerRoot, FAttachmentTransformRules::SnapToTargetIncludingScale);
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
		static_cast<double>(origin.Z)
	);
}

void UCesiumGeoreferenceComponent::_updateAbsoluteLocation() {
	const FVector& relativeLocation = this->_ownerRoot->GetComponentLocation();
	const FIntVector& originLocation = this->GetWorld()->OriginLocation;
	this->_absoluteLocation = glm::dvec3(
		static_cast<double>(originLocation.X) + static_cast<double>(relativeLocation.X),
		static_cast<double>(originLocation.Y) + static_cast<double>(relativeLocation.Y),
		static_cast<double>(originLocation.Z) + static_cast<double>(relativeLocation.Z)
	);
}

void UCesiumGeoreferenceComponent::_updateRelativeLocation() {
	// Note: Since we have a presumably accurate _absoluteLocation, this will be more accurate than querying the floating-point UE relative world 
	// location. This means that although the rendering, physics, and anything else on the UE side might be jittery, our internal representation 
	// of the location will remain accurate.
	this->_relativeLocation = this->_absoluteLocation - this->_worldOriginLocation;
}

void UCesiumGeoreferenceComponent::_initGeoreference() {
	if (this->Georeference) {
		return;
	}
	this->Georeference = ACesiumGeoreference::GetDefaultForActor(this->GetOwner());
	if (this->Georeference) {
		this->_updateActorToECEF(); 
		this->Georeference->AddGeoreferencedObject(this);
	}
	// Note: when a georeferenced object is added, UpdateGeoreferenceTransform will automatically be called
}

void UCesiumGeoreferenceComponent::_updateActorToECEF() {
	if (!this->Georeference) {
		return;
	}
	
	glm::dmat4 georeferencedToEllipsoidCenteredTransform = this->Georeference->GetGeoreferencedToEllipsoidCenteredTransform();

	FMatrix actorToRelativeWorld = this->_ownerRoot->GetComponentToWorld().ToMatrixWithScale();
	glm::dmat4 actorToAbsoluteWorld(
		glm::dvec4(actorToRelativeWorld.M[0][0], actorToRelativeWorld.M[0][1], actorToRelativeWorld.M[0][2], actorToRelativeWorld.M[0][3]),
		glm::dvec4(actorToRelativeWorld.M[1][0], actorToRelativeWorld.M[1][1], actorToRelativeWorld.M[1][2], actorToRelativeWorld.M[1][3]),
		glm::dvec4(actorToRelativeWorld.M[2][0], actorToRelativeWorld.M[2][1], actorToRelativeWorld.M[2][2], actorToRelativeWorld.M[2][3]),
		glm::dvec4(this->_absoluteLocation, 1.0)
	);

	this->_actorToECEF = georeferencedToEllipsoidCenteredTransform * CesiumTransforms::scaleToCesium * CesiumTransforms::unrealToOrFromCesium * actorToAbsoluteWorld;
	this->_updateLongLatHeight();
}

void UCesiumGeoreferenceComponent::_updateActorToUnrealRelativeWorldTransform() {
	if (!this->Georeference) {
		return;
	}
	glm::dmat4 ellipsoidCenteredToGeoreferencedTransform = this->Georeference->GetEllipsoidCenteredToGeoreferencedTransform();
	this->_updateActorToUnrealRelativeWorldTransform(ellipsoidCenteredToGeoreferencedTransform);
}

void UCesiumGeoreferenceComponent::_updateActorToUnrealRelativeWorldTransform(const glm::dmat4& ellipsoidCenteredToGeoreferencedTransform) {
	glm::dmat4 absoluteToRelativeWorld(
		glm::dvec4(1.0, 0.0, 0.0, 0.0),
		glm::dvec4(0.0, 1.0, 0.0, 0.0),
		glm::dvec4(0.0, 0.0, 1.0, 0.0),
		glm::dvec4(-this->_worldOriginLocation, 1.0)
	);

	this->_actorToUnrealRelativeWorld = absoluteToRelativeWorld * CesiumTransforms::unrealToOrFromCesium * CesiumTransforms::scaleToUnrealWorld * ellipsoidCenteredToGeoreferencedTransform * this->_actorToECEF;
}

void UCesiumGeoreferenceComponent::_setTransform(const glm::dmat4& transform) {
	// We are about to get an OnUpdateTransform callback for this, so we preemptively mark down to ignore it.
	_ignoreOnUpdateTransform = true;

	this->_ownerRoot->SetRelativeTransform(FTransform(FMatrix(
		FVector(transform[0].x, transform[0].y, transform[0].z),
		FVector(transform[1].x, transform[1].y, transform[1].z),
		FVector(transform[2].x, transform[2].y, transform[2].z),
		FVector(transform[3].x, transform[3].y, transform[3].z)
	)));
}

void UCesiumGeoreferenceComponent::_updateLongLatHeight() {
	std::optional<CesiumGeospatial::Cartographic> cartographic = CesiumGeospatial::Ellipsoid::WGS84.cartesianToCartographic(this->_actorToECEF[3]);

	if (!cartographic) {
		// only happens when actor is too close to the center of the Earth
		return;
	}

	this->CurrentLongitude = CesiumUtility::Math::radiansToDegrees((*cartographic).longitude);
	this->CurrentLatitude = CesiumUtility::Math::radiansToDegrees((*cartographic).latitude);
	this->CurrentHeight = (*cartographic).height;
}
