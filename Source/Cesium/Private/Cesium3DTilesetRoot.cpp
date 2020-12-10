// Fill out your copyright notice in the Description page of Project Settings.


#include "Cesium3DTilesetRoot.h"
#include "Engine/World.h"
#include "CesiumUtility/Math.h"

// Sets default values for this component's properties
UCesium3DTilesetRoot::UCesium3DTilesetRoot() :
	_originIsRebasing(false),
	_absoluteLocation(0.0, 0.0, 0.0),
	_isDirty(false)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

void UCesium3DTilesetRoot::BeginOriginRebase() {
	this->_originIsRebasing = true;
}

void UCesium3DTilesetRoot::EndOriginRebase() {
	this->_originIsRebasing = false;
}

void UCesium3DTilesetRoot::MarkClean() {
	this->_isDirty = false;
}

// Called when the game starts
void UCesium3DTilesetRoot::BeginPlay()
{
	Super::BeginPlay();

	const FVector& newLocation = this->GetRelativeLocation();
	this->_absoluteLocation = glm::dvec3(
		static_cast<double>(this->GetWorld()->OriginLocation.X) + static_cast<double>(newLocation.X),
		static_cast<double>(this->GetWorld()->OriginLocation.Y) + static_cast<double>(newLocation.Y),
		static_cast<double>(this->GetWorld()->OriginLocation.Z) + static_cast<double>(newLocation.Z)
	);
	this->_isDirty = true;
}

bool UCesium3DTilesetRoot::MoveComponentImpl(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult* OutHit, EMoveComponentFlags MoveFlags, ETeleportType Teleport) {
	bool result = USceneComponent::MoveComponentImpl(Delta, NewRotation, bSweep, OutHit, MoveFlags, Teleport);

	if (this->_originIsRebasing) {
		return result;
	}

	const FVector& newLocation = this->GetRelativeLocation();
	glm::dvec3 newLocationAbsolute(
		static_cast<double>(this->GetWorld()->OriginLocation.X) + static_cast<double>(newLocation.X),
		static_cast<double>(this->GetWorld()->OriginLocation.Y) + static_cast<double>(newLocation.Y),
		static_cast<double>(this->GetWorld()->OriginLocation.Z) + static_cast<double>(newLocation.Z)
	);
	
	// Dirty if the position changes by more than a millimeter.
	this->_isDirty = CesiumUtility::Math::equalsEpsilon(this->_absoluteLocation, newLocationAbsolute, 0.0, 0.001);

	this->_absoluteLocation = newLocationAbsolute;

	return result;
}
