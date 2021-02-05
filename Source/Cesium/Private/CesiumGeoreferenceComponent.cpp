#include "CesiumGeoreferenceComponent.h"

UCesiumGeoreferenceComponent::UCesiumGeoreferenceComponent() {
    this->SetGeoreferenceForOwner();
    this->bAutoActivate = true;

	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
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

	FMatrix componentToWorld = this->GetComponentToWorld().ToMatrixWithScale();
	glm::dmat4 ueAbsoluteToUeLocal = glm::dmat4(
		glm::dvec4(componentToWorld.M[0][0], componentToWorld.M[0][1], componentToWorld.M[0][2], componentToWorld.M[0][3]),
		glm::dvec4(componentToWorld.M[1][0], componentToWorld.M[1][1], componentToWorld.M[1][2], componentToWorld.M[1][3]),
		glm::dvec4(componentToWorld.M[2][0], componentToWorld.M[2][1], componentToWorld.M[2][2], componentToWorld.M[2][3]),
		glm::dvec4(relativeLocation, 1.0)
	);

	glm::dmat4 transform = ueAbsoluteToUeLocal * CesiumTransforms::unrealToOrFromCesium * CesiumTransforms::scaleToUnrealWorld * ellipsoidCenteredToGeoreferencedTransform;

	this->_tilesetToUnrealRelativeWorld = transform;

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