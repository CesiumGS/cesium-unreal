
#include "CesiumGlobeAnchorParent.h"

#include "CesiumGeoreferenceComponent.h"

ACesiumGlobeAnchorParent::ACesiumGlobeAnchorParent() {
	PrimaryActorTick.bCanEverTick = false;

    // don't create the georeference root component if this is a CDO
    this->GeoreferenceComponent = CreateDefaultSubobject<UCesiumGeoreferenceComponent>(TEXT("RootComponent"));
    this->SetRootComponent(this->GeoreferenceComponent);
    this->RootComponent->SetMobility(EComponentMobility::Static);
}

void ACesiumGlobeAnchorParent::OnConstruction(const FTransform& Transform) {

}