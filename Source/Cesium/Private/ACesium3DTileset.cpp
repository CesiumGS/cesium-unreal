// Fill out your copyright notice in the Description page of Project Settings.


#include "ACesium3DTileset.h"
#include "tiny_gltf.h"
#include "UnrealStringConversions.h"
#include "CesiumGltfComponent.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/GameViewportClient.h"
#include "UnrealAssetAccessor.h"
#include "Tileset.h"
#include "Batched3DModelContent.h"
#include "UnrealTaskProcessor.h"

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
	UnrealResourcePreparer(AActor* pActor) : _pActor(pActor) {}

	virtual void prepare(Cesium3DTiles::Tile& tile) {
		Cesium3DTiles::Batched3DModelContent* pB3dm = static_cast<Cesium3DTiles::Batched3DModelContent*>(tile.getContent());
		if (pB3dm) {
			UCesiumGltfComponent::CreateOffGameThread(this->_pActor, pB3dm->gltf(), [&tile](UCesiumGltfComponent* pGltf) {
				tile.finishPrepareRendererResources(pGltf);
			});
		}

	}

	virtual void cancel(Cesium3DTiles::Tile& tile) {

	}

	virtual void free(Cesium3DTiles::Tile& tile, void* pRendererResources) {

	}

private:
	AActor* _pActor;
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
	this->_pTilesetView = &this->_pTileset->createView("default");
}

// Called every frame
void ACesium3DTileset::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Cesium3DTiles::Camera camera;
	const Cesium3DTiles::ViewUpdateResult& result = this->_pTilesetView->update(camera);

	for (Cesium3DTiles::Tile* pTile : result.tilesToRenderThisFrame) {
		Cesium3DTiles::TileContent* pContent = pTile->getContent();
		if (!pContent) {
			continue;
		}

		UCesiumGltfComponent* Gltf = static_cast<UCesiumGltfComponent*>(pTile->getRendererResources());
		this->AddGltf(Gltf);

		// TODO: activate the gltf
	}
}

void ACesium3DTileset::AddGltf(UCesiumGltfComponent* Gltf)
{
	if (Gltf->GetAttachParent() == nullptr) {
		Gltf->AttachToComponent(this->RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	}
}
