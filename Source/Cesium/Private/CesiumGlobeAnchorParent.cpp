
#include "CesiumGlobeAnchorParent.h"

#include "CesiumGeoreferenceComponent.h"

ACesiumGlobeAnchorParent::ACesiumGlobeAnchorParent() {
	PrimaryActorTick.bCanEverTick = false;

    // don't create the georeference root component if this is a CDO
    if (!HasAnyFlags(RF_ClassDefaultObject)) {
        this->_georeferenceComponent = CreateDefaultSubobject<UCesiumGeoreferenceComponent>(TEXT("RootComponent"));
        this->SetRootComponent(this->_georeferenceComponent);
	    this->RootComponent->SetMobility(EComponentMobility::Static);
        this->_georeferenceComponent->SetAutoSnapToEastSouthUp(true);
    }
}

void ACesiumGlobeAnchorParent::OnConstruction(const FTransform& Transform) {

}