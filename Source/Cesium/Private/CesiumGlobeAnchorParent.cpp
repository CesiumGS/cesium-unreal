
#include "CesiumGlobeAnchorParent.h"

#include "CesiumGlobeAnchorComponent.h"

ACesiumGlobeAnchorParent::ACesiumGlobeAnchorParent() {
	PrimaryActorTick.bCanEverTick = false;

    this->RootComponent = this->_globeAnchorComponent = CreateDefaultSubobject<UCesiumGlobeAnchorComponent>(TEXT("GlobeAnchor"));
    // TODO: check if this is CDO before doing the next line
    //this->_globeAnchorComponent->SnapToEastNorthUpTangentPlane();
}

void ACesiumGlobeAnchorParent::OnConstruction(const FTransform& Transform) {

}