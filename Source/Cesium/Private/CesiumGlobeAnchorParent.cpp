
#include "CesiumGlobeAnchorParent.h"

#include "CesiumGeoreferenceComponent.h"

ACesiumGlobeAnchorParent::ACesiumGlobeAnchorParent() {
	PrimaryActorTick.bCanEverTick = false;

    // don't create the georeference root component if this is a CDO
    if (!HasAnyFlags(RF_ClassDefaultObject)) {
        this->GlobeAnchorComponent = CreateDefaultSubobject<UCesiumGeoreferenceComponent>(TEXT("RootComponent"));
        this->SetRootComponent(this->GlobeAnchorComponent);
	    this->RootComponent->SetMobility(EComponentMobility::Static);
        this->_globeAnchorComponent->SnapToWestNorthUpTangentPlane();
    }
}

void ACesiumGlobeAnchorParent::OnConstruction(const FTransform& Transform) {

}