
#include "CesiumGlobeAnchorParent.h"

#include "CesiumGlobeAnchorComponent.h"

ACesiumGlobeAnchorParent::ACesiumGlobeAnchorParent() {
	PrimaryActorTick.bCanEverTick = false;

   this->GlobeAnchorComponent = CreateDefaultSubobject<UCesiumGlobeAnchorComponent>(TEXT("RootComponent"));
   this->SetRootComponent(this->GlobeAnchorComponent);
    // TODO: check if this is CDO before doing the next line
    //this->_globeAnchorComponent->SnapToEastNorthUpTangentPlane();
}

void ACesiumGlobeAnchorParent::OnConstruction(const FTransform& Transform) {

}