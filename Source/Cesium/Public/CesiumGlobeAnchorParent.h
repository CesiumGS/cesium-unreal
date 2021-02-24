#pragma once 

#include "GameFramework/Actor.h"

#include "CesiumGlobeAnchorParent.generated.h"

UCLASS()
class CESIUM_API ACesiumGlobeAnchorParent : public AActor 
{
	GENERATED_BODY()

public:
	ACesiumGlobeAnchorParent();

    UPROPERTY()
    UCesiumGeoreferenceComponent* GeoreferenceComponent;
  
protected:
	virtual void OnConstruction(const FTransform& Transform) override;

};
