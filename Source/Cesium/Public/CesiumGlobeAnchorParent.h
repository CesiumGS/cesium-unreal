#pragma once 

#include "GameFramework/Actor.h"

#include "CesiumGlobeAnchorParent.generated.h"

UCLASS()
class CESIUM_API ACesiumGlobeAnchorParent : public AActor 
{
	GENERATED_BODY()

public:
	ACesiumGlobeAnchorParent();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
    UCesiumGlobeAnchorComponent* _globeAnchorComponent;
};
