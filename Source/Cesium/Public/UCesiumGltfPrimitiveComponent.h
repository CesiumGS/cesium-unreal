// Copyright CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include <glm/mat4x4.hpp>
#include "UCesiumGltfPrimitiveComponent.generated.h"

UCLASS()
class CESIUM_API UCesiumGltfPrimitiveComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCesiumGltfPrimitiveComponent();
	virtual ~UCesiumGltfPrimitiveComponent();

	glm::dmat4x4 HighPrecisionNodeTransform;

	void UpdateTransformFromCesium(const glm::dmat4& cesiumToUnrealTransform);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
