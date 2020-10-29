// Fill out your copyright notice in the Description page of Project Settings.


#include "UCesiumGltfPrimitiveComponent.h"

// Sets default values for this component's properties
UCesiumGltfPrimitiveComponent::UCesiumGltfPrimitiveComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

UCesiumGltfPrimitiveComponent::~UCesiumGltfPrimitiveComponent() {
}

void UCesiumGltfPrimitiveComponent::UpdateTransformFromCesium(const glm::dmat4& cesiumToUnrealTransform) {
	this->SetUsingAbsoluteLocation(true);
	this->SetUsingAbsoluteRotation(true);
	this->SetUsingAbsoluteScale(true);

	const glm::dmat4x4& transform = cesiumToUnrealTransform * this->HighPrecisionNodeTransform;

	this->SetRelativeTransform(FTransform(FMatrix(
		FVector(transform[0].x, transform[0].y, transform[0].z),
		FVector(transform[1].x, transform[1].y, transform[1].z),
		FVector(transform[2].x, transform[2].y, transform[2].z),
		FVector(transform[3].x, transform[3].y, transform[3].z)
	)));
}

// Called when the game starts
void UCesiumGltfPrimitiveComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UCesiumGltfPrimitiveComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

