// Fill out your copyright notice in the Description page of Project Settings.


#include "CesiumGeoreference.h"
#include "Engine/World.h"
#include "CesiumGeospatial/Transforms.h"
#include "CesiumUtility/Math.h"
#include "CesiumGeoreferenceable.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Math/Vector.h"
#include "Math/Rotator.h"
#include "Math/RotationTranslationMatrix.h"
#include "Math/Matrix.h"
#include "CesiumTransforms.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <optional>

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

void ACesiumGeoreference::PlaceGeoreferenceOriginHere() {
	// TODO: check that we are in editor mode, and not play-in-editor mode
	// TODO: should we just assume origin rebasing isn't happening since this is only editor-mode?
	// Or are we even sure this is only for editor-mode?
#if WITH_EDITOR
	glm::dmat4 georeferencedToEllipsoidCenteredTransform = this->GetGeoreferencedToEllipsoidCenteredTransform();

	FViewport* pViewport = GEditor->GetActiveViewport();
	FViewportClient* pViewportClient = pViewport->GetClient();
	FEditorViewportClient* pEditorViewportClient = static_cast<FEditorViewportClient*>(pViewportClient);

	FRotationTranslationMatrix fCameraTransform(
		pEditorViewportClient->GetViewRotation(), 
		pEditorViewportClient->GetViewLocation()
	);
	const FIntVector& originLocation = this->GetWorld()->OriginLocation;

	// TODO: optimize this, only need to transform the front direction and translation

	// camera local space to Unreal absolute world
	glm::dmat4 cameraToAbsolute(
		glm::dvec4(fCameraTransform.M[0][0], fCameraTransform.M[0][1], fCameraTransform.M[0][2], 0.0),
		glm::dvec4(fCameraTransform.M[1][0], fCameraTransform.M[1][1], fCameraTransform.M[1][2], 0.0),
		glm::dvec4(fCameraTransform.M[2][0], fCameraTransform.M[2][1], fCameraTransform.M[2][2], 0.0),
		glm::dvec4(
			static_cast<double>(fCameraTransform.M[3][0]) + static_cast<double>(originLocation.X),
			static_cast<double>(fCameraTransform.M[3][1]) + static_cast<double>(originLocation.Y),
			static_cast<double>(fCameraTransform.M[3][2]) + static_cast<double>(originLocation.Z),
			1.0
		)
	);

	// camera local space to ECEF
	glm::dmat4 cameraToECEF = georeferencedToEllipsoidCenteredTransform * CesiumTransforms::scaleToCesium * CesiumTransforms::unrealToOrFromCesium * cameraToAbsolute;

	// Long/Lat/Height camera location (also our new target georeference origin)
	std::optional<CesiumGeospatial::Cartographic> targetGeoreferenceOrigin = CesiumGeospatial::Ellipsoid::WGS84.cartesianToCartographic(cameraToECEF[3]);

	if (!targetGeoreferenceOrigin) {
		// TODO: should there be some other default behavior here? This only happens when the location is too close to the center of the Earth.
		return;
	}

	this->OriginLongitude = CesiumUtility::Math::radiansToDegrees((*targetGeoreferenceOrigin).longitude);
	this->OriginLatitude = CesiumUtility::Math::radiansToDegrees((*targetGeoreferenceOrigin).latitude);
	this->OriginHeight = (*targetGeoreferenceOrigin).height;

	// TODO: maybe this could eventually be made to happen smoother 
	this->UpdateGeoreference();

	// get the updated ECEF to georeferenced transform
	glm::dmat4 ellipsoidCenteredToGeoreferenced = this->GetEllipsoidCenteredToGeoreferencedTransform();

	glm::dmat4 absoluteToRelativeWorld(
		glm::dvec4(1.0, 0.0, 0.0, 0.0),
		glm::dvec4(0.0, 1.0, 0.0, 0.0),
		glm::dvec4(0.0, 0.0, 1.0, 0.0),
		glm::dvec4(-originLocation.X, -originLocation.Y, -originLocation.Z, 1.0)
	);

	// TODO: check for degeneracy ?
	glm::dmat4 newCameraTransform = absoluteToRelativeWorld * CesiumTransforms::unrealToOrFromCesium * CesiumTransforms::scaleToUnrealWorld * ellipsoidCenteredToGeoreferenced * cameraToECEF;
	glm::dvec3 cameraFront = glm::normalize(newCameraTransform[0]);
	glm::dvec3 cameraRight = glm::normalize(glm::cross(glm::dvec3(0.0, 0.0, 1.0), cameraFront));
	glm::dvec3 cameraUp = glm::normalize(glm::cross(cameraFront, cameraRight));

	pEditorViewportClient->SetViewRotation(
		FMatrix(
			FVector(cameraFront.x, cameraFront.y, cameraFront.z),
			FVector(cameraRight.x, cameraRight.y, cameraRight.z),
			FVector(cameraUp.x, cameraUp.y, cameraUp.z),
			FVector(0.0f, 0.0f, 0.0f)
		).Rotator()
	);	
	pEditorViewportClient->SetViewLocation(FVector(-originLocation.X, -originLocation.Y, -originLocation.Z));
#endif
}

glm::dmat4x4 ACesiumGeoreference::GetGeoreferencedToEllipsoidCenteredTransform() const {
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

glm::dmat4x4 ACesiumGeoreference::GetEllipsoidCenteredToGeoreferencedTransform() const {
	return glm::affineInverse(this->GetGeoreferencedToEllipsoidCenteredTransform());
}

void ACesiumGeoreference::AddGeoreferencedObject(ICesiumGeoreferenceable* Object)
{
	this->_georeferencedObjects.Add(*Object);

	// If this object is an Actor or UActorComponent, make sure it ticks _after_ the CesiumGeoreference.
	AActor* pActor = Cast<AActor>(Object);
	UActorComponent* pActorComponent = Cast<UActorComponent>(Object);
	if (pActor) {
		pActor->AddTickPrerequisiteActor(this);
	} else if (pActorComponent) {
		pActorComponent->AddTickPrerequisiteActor(this);
	}
	
	this->UpdateGeoreference();
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
{}

void ACesiumGeoreference::UpdateGeoreference()
{
	glm::dmat4 transform = this->GetEllipsoidCenteredToGeoreferencedTransform();
	for (TWeakInterfacePtr<ICesiumGeoreferenceable> pObject : this->_georeferencedObjects) {
		if (pObject.IsValid()) {
			pObject->UpdateGeoreferenceTransform(transform);
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

