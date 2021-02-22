
#include "CesiumGlobeAnchorParent.h"

#include "CesiumGlobeAnchorComponent.h"

ACesiumGlobeAnchorParent::ACesiumGlobeAnchorParent() {
	PrimaryActorTick.bCanEverTick = false;

    // don't create the georeference root component if this is a CDO
    if (!HasAnyFlags(RF_ClassDefaultObject)) {
        this->GlobeAnchorComponent = CreateDefaultSubobject<UCesiumGlobeAnchorComponent>(TEXT("RootComponent"));
        this->SetRootComponent(this->GlobeAnchorComponent);
        // TODO: Is this necessarily what we want? Maybe these "anchor actors" can be static while, regular actors using georeference component should be dynamic 
	    //this->RootComponent->SetMobility(EComponentMobility::Static);

        //this->_globeAnchorComponent->SnapToEastNorthUpTangentPlane();
    }
}

void ACesiumGlobeAnchorParent::OnConstruction(const FTransform& Transform) {

}