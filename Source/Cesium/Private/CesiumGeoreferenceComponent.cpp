#include "CesiumGeoreferenceComponent.h"

UCesiumGeoreferenceComponent::UCesiumGeoreferenceComponent() {
    this->SetGeoreferenceForOwner();
    this->bAutoActivate = true;

	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
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

    // TODO: get bounding volume of unreal actor owner
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

	this->_isDirty = true;
}

void UCesiumGeoreferenceComponent::SetGeoreferenceForOwner() {
    if (!_owner) {
        _owner = this->GetOwner();
    }

    if (!_owner) {
        return;
    }

    _georeference = ACesiumGeoreference::GetDefaultForActor(_owner);
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