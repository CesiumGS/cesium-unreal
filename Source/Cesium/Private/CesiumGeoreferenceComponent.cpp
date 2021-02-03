#include "CesiumGeoreferenceComponent.h"

UCesiumGeoreferenceComponent::UCesiumGeoreferenceComponent() {
    _owner = this->GetOwner();
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

// ICesiumGeoreferenceable virtual functions
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

}