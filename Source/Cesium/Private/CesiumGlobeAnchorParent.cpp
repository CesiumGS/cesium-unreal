
#include "CesiumGlobeAnchorParent.h"

#include "Components/SceneComponent.h"
#include "CesiumGeoreferenceComponent.h"

ACesiumGlobeAnchorParent::ACesiumGlobeAnchorParent() {
	PrimaryActorTick.bCanEverTick = true;

    // don't create the georeference root component if this is a CDO
	this->SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent")));
	this->RootComponent->SetMobility(EComponentMobility::Movable);
	this->GeoreferenceComponent = CreateDefaultSubobject<UCesiumGeoreferenceComponent>(TEXT("GeoreferenceComponent"));
}

void ACesiumGlobeAnchorParent::OnConstruction(const FTransform& Transform) {
    Super::OnConstruction(Transform);
	this->GeoreferenceComponent->SetAutoSnapToEastSouthUp(true);
}

bool ACesiumGlobeAnchorParent::ShouldTickIfViewportsOnly() const {
	return true;
}

void ACesiumGlobeAnchorParent::Tick(float DeltaTime) {
	if (this->GeoreferenceComponent->CheckCoordinatesChanged()) {

		this->Longitude = this->GeoreferenceComponent->Longitude;
		this->Latitude = this->GeoreferenceComponent->Latitude;
		this->Altitude = this->GeoreferenceComponent->Altitude;

		this->ECEF_X = this->GeoreferenceComponent->ECEF_X;
		this->ECEF_Y = this->GeoreferenceComponent->ECEF_Y;
		this->ECEF_Z = this->GeoreferenceComponent->ECEF_Z;

		this->GeoreferenceComponent->MarkCoordinatesUnchanged();
	}
}

#if WITH_EDITOR
void ACesiumGlobeAnchorParent::PostEditChangeProperty(FPropertyChangedEvent& event) {
	Super::PostEditChangeProperty(event);

	if (!event.Property) {
		return;
	}

	FName propertyName = event.Property->GetFName();

	if (
		propertyName == GET_MEMBER_NAME_CHECKED(ACesiumGlobeAnchorParent, Longitude) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ACesiumGlobeAnchorParent, Latitude) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ACesiumGlobeAnchorParent, Altitude)
	) {
		this->GeoreferenceComponent->MoveToLongLatAlt(Longitude, Latitude, Altitude);
		return;
	} else if (
		propertyName == GET_MEMBER_NAME_CHECKED(ACesiumGlobeAnchorParent, ECEF_X) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ACesiumGlobeAnchorParent, ECEF_Y) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ACesiumGlobeAnchorParent, ECEF_Z)
	) {
		this->GeoreferenceComponent->MoveToECEF(ECEF_X, ECEF_Y, ECEF_Z);
		return;
	}
}
#endif