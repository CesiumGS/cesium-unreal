#pragma once 

#include "GameFramework/Actor.h"

#include "CesiumGlobeAnchorParent.generated.h"

UCLASS()
class CESIUM_API ACesiumGlobeAnchorParent : public AActor 
{
	GENERATED_BODY()

public:
	ACesiumGlobeAnchorParent();

	/**
	 * Aligns the local up direction with the ellipsoid normal at the current location. 
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Cesium")
	void SnapLocalUpToEllipsoidNormal();

	/**
	 * Aligns the local X, Y, Z axes to East, North, and Up (the ellipsoid normal) respectively.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Cesium")
	void SnapToEastNorthUpTangentPlane(); 

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
    UCesiumGlobeAnchorComponent* _globeAnchorComponent;
};
