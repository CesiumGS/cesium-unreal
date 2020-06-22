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

void AddTiles(ACesium3DTileset& actor, const nlohmann::json& tile, const std::string& baseUrl);

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

	//TGraphTask<int>::FConstructor x = TGraphTask<int>::CreateTask();
	//FGraphEventRef r = x.ConstructAndDispatchWhenReady();

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

	//auto& loaded = this->_loadedProperties;

	//if (loaded.Url == this->Url && loaded.IonAccessToken == this->IonAccessToken && loaded.IonAssetID == this->IonAssetID)
	//{
	//	// Nothing to do!
	//	return;
	//}

	//UE_LOG(LogActor, Warning, TEXT("Deleting old tileset"));
	//if (loaded.request.IsValid())
	//{
	//	loaded.request->OnProcessRequestComplete().Unbind();
	//	loaded.request->CancelRequest();
	//}

	//TArray<USceneComponent*> children;
	//this->RootComponent->GetChildrenComponents(false, children);

	//for (USceneComponent* pComponent : children)
	//{
	//	pComponent->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
	//	pComponent->UnregisterComponent();
	//	pComponent->DestroyComponent(false);
	//}

	//this->RootComponent->GetChildrenComponents(false, children);

	//loaded.Url = this->Url;
	//loaded.IonAssetID = this->IonAssetID;
	//loaded.IonAccessToken = this->IonAccessToken;

	//FHttpModule& httpModule = FHttpModule::Get();
	//loaded.request = httpModule.CreateRequest();

	//if (this->Url.Len() > 0)
	//{
	//	loaded.request->SetURL(this->Url);
	//	loaded.request->OnProcessRequestComplete().BindUObject(this, &ACesium3DTileset::TilesetJsonRequestComplete);
	//}
	//else
	//{
	//	FString url = "https://api.cesium.com/v1/assets/" + FString::FromInt(this->IonAssetID) + "/endpoint";
	//	if (this->IonAccessToken.Len() > 0)
	//	{
	//		url += "?access_token=" + this->IonAccessToken;
	//	}
	//	loaded.request->SetURL(url);
	//	loaded.request->OnProcessRequestComplete().BindUObject(this, &ACesium3DTileset::IonAssetRequestComplete);
	//}

	//loaded.request->ProcessRequest();
}

void ACesium3DTileset::IonAssetRequestComplete(FHttpRequestPtr request, FHttpResponsePtr response, bool x)
{
	//if (response->GetResponseCode() < 200 || response->GetResponseCode() >= 300)
	//{
	//	UE_LOG(
	//		LogActor,
	//		Warning,
	//		TEXT("Error from Cesium ion with HTTP status code %d: %s"),
	//		response->GetResponseCode(),
	//		*response->GetContentAsString());
	//	return;
	//}

	//const TArray<uint8>& content = response->GetContent();

	//using nlohmann::json;
	//json ionResponse = json::parse(&content[0], &content[0] + content.Num());

	//std::string url = ionResponse.value<std::string>("url", "");
	//std::string accessToken = ionResponse.value<std::string>("accessToken", "");
	//std::string urlWithToken = Uri::addQuery(url, "access_token", accessToken);

	//FHttpModule& httpModule = FHttpModule::Get();

	//auto& loaded = this->_loadedProperties;
	//loaded.request = httpModule.CreateRequest();

	//loaded.request->SetURL(utf8_to_wstr(urlWithToken));
	//loaded.request->OnProcessRequestComplete().BindUObject(this, &ACesium3DTileset::TilesetJsonRequestComplete);
	//loaded.request->ProcessRequest();
}

void ACesium3DTileset::TilesetJsonRequestComplete(FHttpRequestPtr request, FHttpResponsePtr response, bool x)
{
	//const TArray<uint8>& content = response->GetContent();

	//using nlohmann::json;
	//json tileset = json::parse(&content[0], &content[0] + content.Num());

	//std::string baseUrl = wstr_to_utf8(request->GetURL());

	//json& root = tileset["root"];
	//AddTiles(*this, root, baseUrl);
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

	//APlayerCameraManager* pCameraManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;

	//UGameViewportClient* pViewport = GetWorld()->GetGameViewport();
	//FVector2D size;
	//pViewport->GetViewportSize(size);

	//UE_LOG(
	//	LogActor,
	//	Warning,
	//	TEXT("FOV %f, Location X: %f, SizeX: %f, SizeY: %f"),
	//	pCameraManager->ViewTarget.POV.FOV,
	//	pCameraManager->ViewTarget.POV.Location.X,
	//	size.X,
	//	size.Y
	//);

	//pCameraManager->ViewTarget.POV.Rotation
}

void ACesium3DTileset::AddGltf(UCesiumGltfComponent* Gltf)
{
	if (Gltf->GetAttachParent() == nullptr) {
		Gltf->AttachToComponent(this->RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	}
}

void AddTiles(ACesium3DTileset& actor, const nlohmann::json& tile, const std::string& baseUrl)
{
	//using nlohmann::json;

	//if (!tile.is_object())
	//{
	//	return;
	//}

	//const bool leavesOnly = true;

	//json::const_iterator contentIt = tile.find("content");
	//json::const_iterator childrenIt = tile.find("children");

	//if (contentIt != tile.end() && (!leavesOnly || childrenIt == tile.end()))
	//{
	//	const std::string& uri = contentIt->value<std::string>("uri", contentIt->value<std::string>("url", ""));
	//	const std::string fullUri = Uri::resolve(baseUrl, uri, true);
	//	
	//	// TODO: we're not supposed to distinguish content type by filename.
	//	//       Instead, load it and look at the content.
	//	if (uri.find(".json") != std::string::npos)
	//	{
	//		return; 

	//		//std::vector<unsigned char> bytes;
	//		//std::string errors;

	//		//std::string externalTilesetPath = Uri::resolve(baseUrl, uri);
	//		//if (!tinygltf::ReadWholeFile(&bytes, &errors, externalTilesetPath, nullptr)) {
	//		//	return;
	//		//}

	//		//using nlohmann::json;
	//		//json externalTileset = json::parse(bytes);

	//		//json& externalRoot = externalTileset["root"];
	//		//AddTiles(actor, externalRoot, dirname(externalTilesetPath));

	//		//return;
	//	}
	//	else
	//	{
	//		const json& content = *contentIt;

	//		// This is a leaf node, add it.
	//		UCesiumGltfComponent* Gltf = NewObject<UCesiumGltfComponent>(&actor);
	//		actor.AddGltf(Gltf);
	//		//Gltf->RegisterComponent();

	//		Gltf->LoadModel(utf8_to_wstr(fullUri));
	//		return;
	//	}
	//}

	//if (childrenIt != tile.end())
	//{
	//	const json& children = *childrenIt;
	//	if (!children.is_array())
	//	{
	//		return;
	//	}

	//	for (auto& child : children)
	//	{
	//		AddTiles(actor, child, baseUrl);
	//	}
	//}
}
