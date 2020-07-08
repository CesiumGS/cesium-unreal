// Fill out your copyright notice in the Description page of Project Settings.


#include "ACesium3DTileset.h"
#include "tiny_gltf.h"
#include "UnrealConversions.h"
#include "CesiumGltfComponent.h"
#include "HttpModule.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/GameViewportClient.h"
#include "UnrealAssetAccessor.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/Batched3DModelContent.h"
#include "UnrealTaskProcessor.h"
#include "Math/UnrealMathUtility.h"
#include "Cesium3DTiles/Transforms.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#pragma warning(push)
#pragma warning(disable: 4946)
#include "json.hpp"
#pragma warning(pop)

// Sets default values
ACesium3DTileset::ACesium3DTileset() :
	_pTileset(nullptr)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	this->RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Tileset"));
	this->RootComponent->SetMobility(EComponentMobility::Static);
}

glm::dmat4x4 ACesium3DTileset::GetWorldToTilesetTransform() const {
	if (!this->PlaceTilesetBoundingVolumeCenterAtWorldOrigin) {
		return glm::dmat4x4(1.0);
	}

	Cesium3DTiles::Tile* pRootTile = this->_pTileset->getRootTile();
	if (!pRootTile) {
		return glm::dmat4x4(1.0);
	}

	const Cesium3DTiles::BoundingVolume& tilesetBoundingVolume = pRootTile->getBoundingVolume();
	glm::dvec3 bvCenter = Cesium3DTiles::getBoundingVolumeCenter(tilesetBoundingVolume);

	if (this->AlignTilesetUpWithZ) {
		return Cesium3DTiles::Transforms::eastNorthUpToFixedFrame(bvCenter);
	}
	else {
		return glm::translate(glm::dmat4x4(1.0), bvCenter);
	}
}

glm::dmat4x4 ACesium3DTileset::GetTilesetToWorldTransform() const
{
	return glm::affineInverse(this->GetWorldToTilesetTransform());
}

// Called when the game starts or when spawned
void ACesium3DTileset::BeginPlay()
{
	Super::BeginPlay();

	this->LoadTileset();
}

void ACesium3DTileset::OnConstruction(const FTransform& Transform)
{
	//this->LoadTileset();
}

class UnrealResourcePreparer : public Cesium3DTiles::IPrepareRendererResources {
public:
	UnrealResourcePreparer(ACesium3DTileset* pActor) : _pActor(pActor) {}

	virtual void prepare(Cesium3DTiles::Tile& tile) {
		Cesium3DTiles::Batched3DModelContent* pB3dm = static_cast<Cesium3DTiles::Batched3DModelContent*>(tile.getContent());
		if (pB3dm) {
			glm::dmat4x4 transform = _pActor->GetTilesetToWorldTransform() * tile.getTransform();
			UCesiumGltfComponent::CreateOffGameThread(this->_pActor, pB3dm->gltf(), transform, [&tile](UCesiumGltfComponent* pGltf) {
				tile.finishPrepareRendererResources(pGltf);
			});
		}

	}

	virtual void cancel(Cesium3DTiles::Tile& tile) {

	}

	virtual void free(Cesium3DTiles::Tile& tile, void* pRendererResources) {

	}

private:
	ACesium3DTileset* _pActor;
};

void ACesium3DTileset::LoadTileset()
{
	Cesium3DTiles::Tileset* pTileset = this->_pTileset;
	if (pTileset)
	{
		if (this->Url.Len() > 0)
		{
			if (pTileset->getUrl() && wstr_to_utf8(this->Url) == pTileset->getUrl())
			{
				// Already using this URL.
				return;
			}
		}
		else
		{
			if (pTileset->getIonAssetID() && pTileset->getIonAccessToken() && this->IonAssetID == pTileset->getIonAssetID() && wstr_to_utf8(this->IonAccessToken) == pTileset->getIonAccessToken())
			{
				// Already using this asset ID and access token.
				return;
			}
		}
	}

	Cesium3DTiles::TilesetExternals externals{
		new UnrealAssetAccessor(),
		new UnrealResourcePreparer(this),
		new UnrealTaskProcessor()
	};

	if (this->Url.Len() > 0)
	{
		pTileset = new Cesium3DTiles::Tileset(externals, wstr_to_utf8(this->Url));
	}
	else
	{
		pTileset = new Cesium3DTiles::Tileset(externals, this->IonAssetID, wstr_to_utf8(this->IonAccessToken));
	}

	this->_pTileset = pTileset;
}

// Called every frame
void ACesium3DTileset::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (this->SuspendUpdate) {
		return;
	}

	Cesium3DTiles::Tile* pRootTile = this->_pTileset->getRootTile();
	if (!pRootTile) {
		return;
	}


	APlayerCameraManager* pCameraManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;

	UGameViewportClient* pViewport = GetWorld()->GetGameViewport();

	FVector2D size;
	pViewport->GetViewportSize(size);

	const FMinimalViewInfo& pov = pCameraManager->ViewTarget.POV;
	const FVector& location = pov.Location;

	double horizontalFieldOfView = FMath::DegreesToRadians(pov.FOV);

	double aspectRatio = size.X / size.Y;
	double verticalFieldOfView = atan(tan(horizontalFieldOfView * 0.5) / aspectRatio) * 2.0;

	FVector direction = pCameraManager->ViewTarget.POV.Rotation.RotateVector(FVector(1.0f, 0.0f, 0.0f));
		
	FVector up = pCameraManager->ViewTarget.POV.Rotation.RotateVector(FVector(0.0f, 0.0f, 1.0f));

	auto tryTransform = [](const FVector& v) {
		return glm::dvec3(v.X, -v.Y, v.Z);
	};

	const FTransform& tilesetToWorld = this->RootComponent->GetComponentToWorld();
	FVector locationRelativeToTileset = tilesetToWorld.InverseTransformPosition(location);
	FVector directionRelativeToTileset = tilesetToWorld.InverseTransformVector(direction);
	FVector upRelativeToTileset = tilesetToWorld.InverseTransformVector(up);

	glm::dvec3 cesiumPosition = tryTransform(locationRelativeToTileset) / 100.0;
	glm::dvec3 cesiumDirection = tryTransform(directionRelativeToTileset);
	glm::dvec3 cesiumUp = tryTransform(upRelativeToTileset);

	glm::dmat4x4 transform = this->GetWorldToTilesetTransform();

	Cesium3DTiles::Camera camera(
		transform * glm::dvec4(cesiumPosition, 1.0),
		transform * glm::dvec4(cesiumDirection, 0.0),
		transform * glm::dvec4(cesiumUp, 0.0),
		glm::dvec2(size.X, size.Y),
		horizontalFieldOfView,
		verticalFieldOfView
	);

	const Cesium3DTiles::ViewUpdateResult& result = this->_pTileset->updateView(camera);

	for (Cesium3DTiles::Tile* pTile : result.tilesToNoLongerRenderThisFrame) {
		UCesiumGltfComponent* Gltf = static_cast<UCesiumGltfComponent*>(pTile->getRendererResources());
		if (Gltf) {
			Gltf->SetVisibility(false, true);
		} else {
			// TODO: why is this happening?
		}
	}

	for (Cesium3DTiles::Tile* pTile : result.tilesToRenderThisFrame) {
		Cesium3DTiles::TileContent* pContent = pTile->getContent();
		if (!pContent) {
			continue;
		}

		UCesiumGltfComponent* Gltf = static_cast<UCesiumGltfComponent*>(pTile->getRendererResources());
		if (!Gltf) {
			// TODO: Not-yet-renderable tiles shouldn't be here.
			continue;
		}

		if (Gltf->GetAttachParent() == nullptr) {
			Gltf->AttachToComponent(this->RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		}

		Gltf->SetVisibility(true, true);
	}
}
