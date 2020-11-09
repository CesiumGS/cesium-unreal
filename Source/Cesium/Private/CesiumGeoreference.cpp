// Fill out your copyright notice in the Description page of Project Settings.


#include "CesiumGeoreference.h"
#include "Engine/World.h"
#include "CesiumGeospatial/Transforms.h"
#include "CesiumGeoreferenceable.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "CesiumTransforms.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

/*static*/ ACesiumGeoreference* ACesiumGeoreference::GetDefaultForActor(AActor* Actor) {
	ACesiumGeoreference* pGeoreference = FindObject<ACesiumGeoreference>(Actor->GetLevel(), TEXT("CesiumGeoreferenceDefault"));
	if (!pGeoreference) {
		FActorSpawnParameters spawnParameters;
		spawnParameters.Name = TEXT("CesiumGeoreferenceDefault");
		spawnParameters.OverrideLevel = Actor->GetLevel();
		pGeoreference = Actor->GetWorld()->SpawnActor<ACesiumGeoreference>(spawnParameters);
	}
	return pGeoreference;
}

ACesiumGeoreference::ACesiumGeoreference()
{
	PrimaryActorTick.bCanEverTick = true;
}

glm::dmat4x4 ACesiumGeoreference::GetCurrentCesiumToUnrealLocalTransform() const
{
	// TODO: cache this
	return this->GetNextCesiumToUnrealLocalTransform(FIntVector(0, 0, 0));
}

glm::dmat4x4 ACesiumGeoreference::GetNextCesiumToUnrealLocalTransform(const FIntVector& WorldOriginOffset) const
{
	const FIntVector& oldOrigin = this->GetWorld()->OriginLocation;
	glm::dvec3 originLocation = glm::dvec3(
		static_cast<double>(oldOrigin.X) - static_cast<double>(WorldOriginOffset.X),
		static_cast<double>(oldOrigin.Y) - static_cast<double>(WorldOriginOffset.Y),
		static_cast<double>(oldOrigin.Z) - static_cast<double>(WorldOriginOffset.Z)
	);

	glm::dmat4 globalToLocal = glm::translate(
		glm::dmat4x4(1.0),
		glm::dvec3(-originLocation.x, originLocation.y, -originLocation.z) / 100.0
	);

	glm::dmat4 tilesetToWorld = this->GetEllipsoidCenteredToAbsoluteUnrealWorldTransform();
	glm::dmat4 cesiumToUnrealTransform = CesiumTransforms::unrealToOrFromCesium * CesiumTransforms::scaleToUnrealWorld * globalToLocal * tilesetToWorld;
	return cesiumToUnrealTransform;
}

glm::dmat4x4 ACesiumGeoreference::GetAbsoluteUnrealWorldToEllipsoidCenteredTransform() const {
	if (this->OriginPlacement == EOriginPlacement::TrueOrigin) {
		return glm::dmat4(1.0);
	}

	glm::dvec3 center(0.0, 0.0, 0.0);

	if (this->OriginPlacement == EOriginPlacement::BoundingVolumeOrigin) {
		// TODO: it'd be better to compute the union of the bounding volumes and then use the union's center,
		//       rather than averaging the centers.
		size_t numberOfPositions = 0;

		for (const TWeakInterfacePtr<ICesiumGeoreferenceable> pObject : this->_georeferencedObjects) {
			if (pObject.IsValid() && pObject->IsBoundingVolumeReady()) {
				std::optional<Cesium3DTiles::BoundingVolume> bv = pObject->GetBoundingVolume();
				if (bv) {
					center += Cesium3DTiles::getBoundingVolumeCenter(*bv);
					++numberOfPositions;
				}
			}
		}

		if (numberOfPositions > 0) {
			center /= numberOfPositions;
		}
	}
	else if (this->OriginPlacement == EOriginPlacement::CartographicOrigin) {
		const CesiumGeospatial::Ellipsoid& ellipsoid = CesiumGeospatial::Ellipsoid::WGS84;
		center = ellipsoid.cartographicToCartesian(CesiumGeospatial::Cartographic::fromDegrees(this->OriginLongitude, this->OriginLatitude, this->OriginHeight));
	}

	if (this->AlignTilesetUpWithZ) {
		return CesiumGeospatial::Transforms::eastNorthUpToFixedFrame(center);
	} else {
		return glm::translate(glm::dmat4(1.0), center);
	}
}

glm::dmat4x4 ACesiumGeoreference::GetEllipsoidCenteredToAbsoluteUnrealWorldTransform() const {
	return glm::affineInverse(this->GetAbsoluteUnrealWorldToEllipsoidCenteredTransform());
}

void ACesiumGeoreference::AddGeoreferencedObject(ICesiumGeoreferenceable* Object)
{
	this->_georeferencedObjects.Add(*Object);

	// If this object is an Actor, make sure it ticks _after_ the CesiumGeoreference.
	AActor* pActor = Cast<AActor>(Object);
	if (pActor) {
		pActor->AddTickPrerequisiteActor(this);
	}
}

// Called when the game starts or when spawned
void ACesiumGeoreference::BeginPlay()
{
	Super::BeginPlay();
	
	if (this->KeepWorldOriginNearCamera && !this->WorldOriginCamera) {
		// Find the first player's camera manager
		APlayerController* pPlayerController = GetWorld()->GetFirstPlayerController();
		if (pPlayerController) {
			this->WorldOriginCamera = pPlayerController->PlayerCameraManager;
		}
	}
}

void ACesiumGeoreference::OnConstruction(const FTransform& Transform)
{
	this->UpdateGeoreference();
}

void ACesiumGeoreference::UpdateGeoreference()
{
	glm::dmat4 cesiumToUnreal = this->GetCurrentCesiumToUnrealLocalTransform();
	for (TWeakInterfacePtr<ICesiumGeoreferenceable> pObject : this->_georeferencedObjects) {
		if (pObject.IsValid()) {
			pObject->UpdateTransformFromCesium(cesiumToUnreal);
		}
	}
}

#if WITH_EDITOR
void ACesiumGeoreference::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	this->UpdateGeoreference();
}
#endif

// Called every frame
void ACesiumGeoreference::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (this->KeepWorldOriginNearCamera && this->WorldOriginCamera) {
		const FMinimalViewInfo& pov = this->WorldOriginCamera->ViewTarget.POV;
		const FVector& cameraLocation = pov.Location;

		if (!cameraLocation.Equals(FVector(0.0f, 0.0f, 0.0f), this->MaximumWorldOriginDistanceFromCamera)) {
			// Camera has moved too far from the origin, move the origin.
			const FIntVector& originLocation = this->GetWorld()->OriginLocation;

			this->GetWorld()->SetNewWorldOrigin(FIntVector(
				static_cast<int32>(cameraLocation.X) + static_cast<int32>(originLocation.X),
				static_cast<int32>(cameraLocation.Y) + static_cast<int32>(originLocation.Y),
				static_cast<int32>(cameraLocation.Z) + static_cast<int32>(originLocation.Z)
			));
		}
	}
}

