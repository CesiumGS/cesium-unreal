#include "CesiumGeoreferenceComponent.h"

UCesiumGeoreferenceComponent::UCesiumGeoreferenceComponent() :
    _worldOriginLocation(0.0),
    _absoluteLocation(0.0),
    _actorToUnrealRelativeWorld(),
    _isDirty(false)
{
    this->_setGeoreference();
    this->bAutoActivate = true;
	PrimaryComponentTick.bCanEverTick = false;
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

	this->_updateTilesetToUnrealRelativeWorldTransform();
}

bool UCesium3DTilesetRoot::MoveComponentImpl(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult* OutHit, EMoveComponentFlags MoveFlags, ETeleportType Teleport) {
	bool result = USceneComponent::MoveComponentImpl(Delta, NewRotation, bSweep, OutHit, MoveFlags, Teleport);

	this->_updateAbsoluteLocation();
	this->_updateTilesetToUnrealRelativeWorldTransform();

	return result;
}

void UCesiumGeoreferenceComponent::BeginPlay() {
    Super::BeginPlay();
}

void UCesiumGeoreferenceComponent::Activate(bool bReset) 
{

}

void UCesiumGeoreferenceComponent::Deactivate() 
{

}

void UCesiumGeoreferenceComponent::OnComponentDestroyed(bool bDestroyingHierarchy) 
{

}

bool UCesiumGeoreferenceComponent::IsBoundingVolumeReady() const
{
    // TODO: get bounding volume of unreal actor owner
    return false;
}

std::optional<Cesium3DTiles::BoundingVolume> UCesiumGeoreferenceComponent::GetBoundingVolume() const 
{
    if (!this->IsBoundingVolumeReady()) {
        return std::nullopt;
    }

    // TODO: get bounding volume of unreal actor owner (must be in cesium coordinates)
    return std::nullopt;
}

void UCesiumGeoreferenceComponent::UpdateGeoreferenceTransform(const glm::dmat4& ellipsoidCenteredToGeoreferencedTransform) 
{
    glm::dvec3 relativeLocation = this->_absoluteLocation - this->_worldOriginLocation;

    // TODO: deprecated? GetComponentTransform() instead?
	FMatrix actorToRelativeWorld = this->GetComponentToWorld().ToMatrixWithScale();
	glm::dmat4 ueAbsoluteToUeRelative = glm::dmat4(
		glm::dvec4(actorToRelativeWorld.M[0][0], actorToRelativeWorld.M[0][1], actorToRelativeWorld.M[0][2], actorToRelativeWorld.M[0][3]),
		glm::dvec4(actorToRelativeWorld.M[1][0], actorToRelativeWorld.M[1][1], actorToRelativeWorld.M[1][2], actorToRelativeWorld.M[1][3]),
		glm::dvec4(actorToRelativeWorld.M[2][0], actorToRelativeWorld.M[2][1], actorToRelativeWorld.M[2][2], actorToRelativeWorld.M[2][3]),
		glm::dvec4(relativeLocation, 1.0)
	);

    // NOTE: ECEF --> Georeferenced is calculated in cesium coordinates 
    // TODO: simplify this somehow
	glm::dmat4 transform = ueAbsoluteToUeRelative * CesiumTransforms::unrealToOrFromCesium * CesiumTransforms::scaleToUnrealWorld * ellipsoidCenteredToGeoreferencedTransform * CesiumTransforms::unrealToOrFromCesium * CesiumTransforms::scaleToCesium;

	this->_actorToUnrealRelativeWorld = transform;

    // TODO: probably don't need this now
	this->_isDirty = true;
}

void UCesiumGeoreferenceComponent::UpdateTransformFromCesium(const glm::dmat4& cesiumToUnrealTransform) {
	this->SetUsingAbsoluteLocation(true);
	this->SetUsingAbsoluteRotation(true);
	this->SetUsingAbsoluteScale(true);

	const glm::dmat4x4& transform = cesiumToUnrealTransform * this->HighPrecisionNodeTransform;

	this->SetRelativeTransform(FTransform(FMatrix(
		FVector(transform[0].x, transform[0].y, transform[0].z),
		FVector(transform[1].x, transform[1].y, transform[1].z),
		FVector(transform[2].x, transform[2].y, transform[2].z),
		FVector(transform[3].x, transform[3].y, transform[3].z)
	)));
}

void UCesiumGeoreferenceComponent::_setGeoreference() {
    _georeference = ACesiumGeoreference::GetDefaultForActor(this->GetOwner());
}

void UCesium3DTilesetRoot::_updateAbsoluteLocation() {
	const FVector& newLocation = this->GetRelativeLocation();
	const FIntVector& originLocation = this->GetWorld()->OriginLocation;
	this->_absoluteLocation = glm::dvec3(
		static_cast<double>(originLocation.X) + static_cast<double>(newLocation.X),
		static_cast<double>(originLocation.Y) + static_cast<double>(newLocation.Y),
		static_cast<double>(originLocation.Z) + static_cast<double>(newLocation.Z)
	);
}
