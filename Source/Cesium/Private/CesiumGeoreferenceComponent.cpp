#include "CesiumGeoreferenceComponent.h"
#include "Engine/EngineTypes.h"
#include "CesiumTransforms.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include <glm/gtc/matrix_inverse.hpp>

UCesiumGeoreferenceComponent::UCesiumGeoreferenceComponent() :
    _worldOriginLocation(0.0),
    _absoluteLocation(0.0),
	_relativeLocation(0.0),
	_actorToECEF(),
    _actorToUnrealRelativeWorld(),
	_ownerRoot(nullptr)
{
    this->bAutoActivate = true;
	PrimaryComponentTick.bCanEverTick = false;
	// TODO: check if we can attach to ownerRoot here instead of OnRegister
}

void UCesiumGeoreferenceComponent::SnapLocalUpToEllipsoidNormal() {
	if (!this->Georeference) {
		return;
	}

	// Note: This is an aggressively optimized implementation of Rodrigues' rotation formula to rotate the actor as to align local-up with the ellipsoid normal.

	// local up in ECEF
	glm::dvec3 actorUpECEF = glm::normalize(this->_actorToECEF[2]);
	// actor location in ECEF
	glm::dvec3 actorLocationECEF = this->_actorToECEF[3];
	
	// the surface normal of the ellipsoid model of the globe at the ECEF location of the actor
	glm::dvec3 ellipsoidNormal = CesiumGeospatial::Ellipsoid::WGS84.geodeticSurfaceNormal(actorLocationECEF);
	
	// the axis of the shortest available rotation
	glm::dvec3 axis = glm::cross(actorUpECEF, ellipsoidNormal);
	// cosine of the angle between the actor's up direction and the ellipsoid normal
	double cos = glm::dot(actorUpECEF, ellipsoidNormal);

	// TODO: use an appropriate Cesium epsilon
	if (cos < -0.999) {
		// The actor's current up direction is completely upside down with respect to the ellipsoid normal.
		
		// We want to do a 180 degree rotation around X. We can do this by flipping the Y and Z axes 
		this->_actorToECEF[1] *= -1.0;
		this->_actorToECEF[2] *= -1.0;
	} else {
		// We know the new Z direction should be the ellipsoid normal	
		this->_actorToECEF[2] = glm::vec4(ellipsoidNormal, 0.0);

		// Instead of constructing a rotation matrix, we can choose one basis vector in particular to rotate (X in this case) since we already know the new Z.
		// This is Rodrigues' rotation formula for rotating a single vector by theta around an axis. Note that we can skip out on a lot of trigonometric
		// calculations due to the fact we had vectors representing a before / after of the rotation and we were able to compute the axis and cosine from those.
		// TODO: add reference link??
		glm::dvec3 x = this->_actorToECEF[0];
		glm::dvec3 axis_cross_x = glm::cross(axis, x);
		x += axis_cross_x + glm::cross(axis, axis_cross_x) / (1.0 + cos);
		this->_actorToECEF[0] = glm::vec4(x, 0.0);

		// Now we can cross Z with X to get Y
		this->_actorToECEF[1] = glm::vec4(glm::cross(ellipsoidNormal, x), 0.0);
	}

	this->_updateActorToUnrealRelativeWorldTransform();
	this->_setTransform(this->_actorToUnrealRelativeWorld);
}

// TODO: is this really the best place to do this?
void UCesiumGeoreferenceComponent::OnRegister() {
	Super::OnRegister();

	AActor* owner = this->GetOwner();
	this->_ownerRoot = owner->GetRootComponent();
	this->AttachToComponent(this->_ownerRoot, FAttachmentTransformRules::SnapToTargetIncludingScale);

	this->_updateAbsoluteLocation();
	this->_updateRelativeLocation();
	this->_initGeoreference();
}

// TODO: this problem probably also exists in TilesetRoot, but if an origin rebase has happened before placing the actor (or tileset) the internal 
// world origin might be incorrect?

// TODO: maybe this calculation can happen solely on the Georeference actor who can maintain a double precision consensus of the worldOrigin
void UCesiumGeoreferenceComponent::ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) {
	USceneComponent::ApplyWorldOffset(InOffset, bWorldShift);

	// TODO:
	// UE_LOG here to check if this is getting called (pretty sure it is)

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

// TODO: does this account for all movement, or only editor movement? maybe need to do the same thing in other overriden functions
bool UCesiumGeoreferenceComponent::MoveComponentImpl(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult* OutHit, EMoveComponentFlags MoveFlags, ETeleportType Teleport) {
	bool result = USceneComponent::MoveComponentImpl(Delta, NewRotation, bSweep, OutHit, MoveFlags, Teleport);

	this->_updateAbsoluteLocation();
	this->_updateRelativeLocation();
	this->_updateActorToECEF();
	this->_updateActorToUnrealRelativeWorldTransform();

	return result;
}

void UCesiumGeoreferenceComponent::BeginPlay() {
    Super::BeginPlay();
}

void UCesiumGeoreferenceComponent::Activate(bool bReset) {

}

void UCesiumGeoreferenceComponent::Deactivate() {

}

void UCesiumGeoreferenceComponent::OnComponentDestroyed(bool bDestroyingHierarchy) {
	Super::OnComponentDestroyed(bDestroyingHierarchy);
	// TODO: remove from georeferenced list ?
}

bool UCesiumGeoreferenceComponent::IsBoundingVolumeReady() const {
    // TODO: get bounding volume of unreal actor owner
    return false;
}

std::optional<Cesium3DTiles::BoundingVolume> UCesiumGeoreferenceComponent::GetBoundingVolume() const {
    if (!this->IsBoundingVolumeReady()) {
        return std::nullopt;
    }

    // TODO: get bounding volume of unreal actor owner (must be in cesium coordinates)
    return std::nullopt;
}

void UCesiumGeoreferenceComponent::UpdateGeoreferenceTransform(const glm::dmat4& ellipsoidCenteredToGeoreferencedTransform) {
	this->_updateActorToUnrealRelativeWorldTransform(ellipsoidCenteredToGeoreferencedTransform);
	this->_setTransform(this->_actorToUnrealRelativeWorld);
}


void UCesiumGeoreferenceComponent::_updateAbsoluteLocation() {
	// TODO: I think we should use GetComponentLocation() here, it is much more clear in the context of a root component 
	// relative location happens to be world location here since this is a root component, but it does not mean relative world...
	// it means relative to parent
	const FVector& relativeLocation = this->_ownerRoot->GetComponentLocation();//GetRelativeLocation();
	const FIntVector& originLocation = this->GetWorld()->OriginLocation;
	this->_absoluteLocation = glm::dvec3(
		static_cast<double>(originLocation.X) + static_cast<double>(relativeLocation.X),
		static_cast<double>(originLocation.Y) + static_cast<double>(relativeLocation.Y),
		static_cast<double>(originLocation.Z) + static_cast<double>(relativeLocation.Z)
	);
}

void UCesiumGeoreferenceComponent::_updateRelativeLocation() {
	// Note: We are tracking this instead of using the floating-point UE relative world location, since this will be more accurate.
	// This means that while the rendering, physics, and anything else on the UE side might be lossy, our internal representation of the location will remain accurate.
	this->_relativeLocation = this->_absoluteLocation - this->_worldOriginLocation;
}

void UCesiumGeoreferenceComponent::_initGeoreference() {
    this->Georeference = ACesiumGeoreference::GetDefaultForActor(this->GetOwner());
	if (this->Georeference) {
		this->_updateActorToECEF(); 
		this->Georeference->AddGeoreferencedObject(this);
	}
	// Note: when a georeferenced object is added, UpdateGeoreferenceTransform will automatically be called
}

// TODO: maybe change this to _updateActorToEllipsoidCentered
void UCesiumGeoreferenceComponent::_updateActorToECEF() {
	if (!this->Georeference) {
		return;
	}
	// TODO: we should avoid duplicate computation from Georeferenced -> ECEF vs ECEF -> Georeferenced, they are computed identically plus inverting in Georeference
	glm::dmat4 georeferencedToEllipsoidCenteredTransform = this->Georeference->GetGeoreferencedToEllipsoidCenteredTransform();

	FMatrix actorToRelativeWorld = this->_ownerRoot->GetComponentToWorld().ToMatrixWithScale();

	glm::dmat4 actorToAbsoluteWorld(
		glm::dvec4(actorToRelativeWorld.M[0][0], actorToRelativeWorld.M[0][1], actorToRelativeWorld.M[0][2], actorToRelativeWorld.M[0][3]),
		glm::dvec4(actorToRelativeWorld.M[1][0], actorToRelativeWorld.M[1][1], actorToRelativeWorld.M[1][2], actorToRelativeWorld.M[1][3]),
		glm::dvec4(actorToRelativeWorld.M[2][0], actorToRelativeWorld.M[2][1], actorToRelativeWorld.M[2][2], actorToRelativeWorld.M[2][3]),
		glm::dvec4(this->_absoluteLocation, 1.0)
	);

	this->_actorToECEF = georeferencedToEllipsoidCenteredTransform * CesiumTransforms::scaleToCesium * CesiumTransforms::unrealToOrFromCesium * actorToAbsoluteWorld;
}

void UCesiumGeoreferenceComponent::_updateActorToUnrealRelativeWorldTransform() {
	if (!this->Georeference) {
		return;
	}
	// TODO: check if this can be precomputed on the Georeference side
	glm::dmat4 ellipsoidCenteredToGeoreferencedTransform = this->Georeference->GetEllipsoidCenteredToGeoreferencedTransform();
	this->_updateActorToUnrealRelativeWorldTransform(ellipsoidCenteredToGeoreferencedTransform);
}

void UCesiumGeoreferenceComponent::_updateActorToUnrealRelativeWorldTransform(const glm::dmat4& ellipsoidCenteredToGeoreferencedTransform) {
    // TODO: deprecated? GetComponentTransform() instead?
	FMatrix actorToRelativeWorld = this->_ownerRoot->GetComponentToWorld().ToMatrixWithScale();

	// Note: We are using single-precision rotation / scaling, but double-precision translation. This matrix is the same as actorToRelativeWorld except for the extra precision in translation.
	glm::dmat4 preciseActorToRelativeWorld = glm::dmat4(
		glm::dvec4(actorToRelativeWorld.M[0][0], actorToRelativeWorld.M[0][1], actorToRelativeWorld.M[0][2], actorToRelativeWorld.M[0][3]),
		glm::dvec4(actorToRelativeWorld.M[1][0], actorToRelativeWorld.M[1][1], actorToRelativeWorld.M[1][2], actorToRelativeWorld.M[1][3]),
		glm::dvec4(actorToRelativeWorld.M[2][0], actorToRelativeWorld.M[2][1], actorToRelativeWorld.M[2][2], actorToRelativeWorld.M[2][3]),
		glm::dvec4(this->_relativeLocation, 1.0)
	);

	glm::dmat4 absoluteToRelativeWorld(
		glm::dvec4(1.0, 0.0, 0.0, 0.0),
		glm::dvec4(0.0, 1.0, 0.0, 0.0),
		glm::dvec4(0.0, 0.0, 1.0, 0.0),
		glm::dvec4(-this->_worldOriginLocation, 1.0)
	);

    // Note: ECEF --> Georeferenced is calculated in cesium coordinates 
    // TODO: what we really need is, actor (ueAbs) -> ?? -> -> ECEF -> Georef -> ueAbsolute -> ueRelative
	this->_actorToUnrealRelativeWorld = /*preciseActorToRelativeWorld */absoluteToRelativeWorld * CesiumTransforms::unrealToOrFromCesium * CesiumTransforms::scaleToUnrealWorld * ellipsoidCenteredToGeoreferencedTransform * this->_actorToECEF;
										//CesiumTransforms::scaleToCesium * CesiumTransforms::unrealToOrFromCesium *
										//ueRelativeToUeAbsolute;

	// * CesiumTransforms::scaleToCesium * CesiumTransforms::unrealToOrFromCesium;
	// TODO: is this->_actorToUnealRelativeWorld needed? 
	//this->_setTransform(this->_actorToUnrealRelativeWorld);
}

void UCesiumGeoreferenceComponent::_setTransform(const glm::dmat4& transform) {
	this->_ownerRoot->SetRelativeTransform(FTransform(FMatrix(
		FVector(transform[0].x, transform[0].y, transform[0].z),
		FVector(transform[1].x, transform[1].y, transform[1].z),
		FVector(transform[2].x, transform[2].y, transform[2].z),
		FVector(transform[3].x, transform[3].y, transform[3].z)
	)));
}
