// Fill out your copyright notice in the Description page of Project Settings.


#include "Cesium3DTileset.h"
#include "json.hpp"
#include "tiny_gltf.h"
#include "UnrealStringConversions.h"
#include "CesiumGltfComponent.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Uri.h"

void AddTiles(ACesium3DTileset& actor, const nlohmann::json& tile, const std::string& baseUrl);

// Sets default values
ACesium3DTileset::ACesium3DTileset()
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

	FHttpModule& httpModule = FHttpModule::Get();
	FHttpRequestRef request = httpModule.CreateRequest();

	if (this->Url.Len() > 0)
	{
		request->SetURL(this->Url);
		request->OnProcessRequestComplete().BindUObject(this, &ACesium3DTileset::TilesetJsonRequestComplete);
	}
	else
	{
		FString url = "https://api.cesium.com/v1/assets/" + FString::FromInt(this->IonAssetID) + "/endpoint";
		if (this->IonAccessToken.Len() > 0)
		{
			url += "?access_token=" + this->IonAccessToken;
		}
		request->SetURL(url);
		request->OnProcessRequestComplete().BindUObject(this, &ACesium3DTileset::IonAssetRequestComplete);
	}

	request->ProcessRequest();
}

void ACesium3DTileset::IonAssetRequestComplete(FHttpRequestPtr request, FHttpResponsePtr response, bool x)
{
	const TArray<uint8>& content = response->GetContent();

	using nlohmann::json;
	json ionResponse = json::parse(&content[0], &content[0] + content.Num());

	std::string url = ionResponse.value<std::string>("url", "");
	std::string accessToken = ionResponse.value<std::string>("accessToken", "");
	std::string urlWithToken = Uri::addQuery(url, "access_token", accessToken);

	FHttpModule& httpModule = FHttpModule::Get();
	FHttpRequestRef nextRequest = httpModule.CreateRequest();

	nextRequest->SetURL(utf8_to_wstr(urlWithToken));
	nextRequest->OnProcessRequestComplete().BindUObject(this, &ACesium3DTileset::TilesetJsonRequestComplete);
	nextRequest->ProcessRequest();
}

void ACesium3DTileset::TilesetJsonRequestComplete(FHttpRequestPtr request, FHttpResponsePtr response, bool x)
{
	const TArray<uint8>& content = response->GetContent();

	using nlohmann::json;
	json tileset = json::parse(&content[0], &content[0] + content.Num());

	std::string baseUrl = wstr_to_utf8(request->GetURL());

	json& root = tileset["root"];
	AddTiles(*this, root, baseUrl);
}

// Called every frame
void ACesium3DTileset::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACesium3DTileset::AddGltf(UCesiumGltfComponent* Gltf)
{
	Gltf->AttachToComponent(this->RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
}

void AddTiles(ACesium3DTileset& actor, const nlohmann::json& tile, const std::string& baseUrl)
{
	using nlohmann::json;

	if (!tile.is_object())
	{
		return;
	}

	const bool leavesOnly = true;

	json::const_iterator contentIt = tile.find("content");
	json::const_iterator childrenIt = tile.find("children");

	if (contentIt != tile.end() && (!leavesOnly || childrenIt == tile.end()))
	{
		const std::string& uri = contentIt->value<std::string>("uri", contentIt->value<std::string>("url", ""));
		const std::string fullUri = Uri::resolve(baseUrl, uri, true);
		
		// TODO: we're not supposed to distinguish content type by filename.
		//       Instead, load it and look at the content.
		if (uri.find(".json") != std::string::npos)
		{
			return; 

			//std::vector<unsigned char> bytes;
			//std::string errors;

			//std::string externalTilesetPath = Uri::resolve(baseUrl, uri);
			//if (!tinygltf::ReadWholeFile(&bytes, &errors, externalTilesetPath, nullptr)) {
			//	return;
			//}

			//using nlohmann::json;
			//json externalTileset = json::parse(bytes);

			//json& externalRoot = externalTileset["root"];
			//AddTiles(actor, externalRoot, dirname(externalTilesetPath));

			//return;
		}
		else
		{
			const json& content = *contentIt;

			// This is a leaf node, add it.
			UCesiumGltfComponent* Gltf = NewObject<UCesiumGltfComponent>(&actor);
			actor.AddGltf(Gltf);
			//Gltf->RegisterComponent();

			Gltf->LoadModel(utf8_to_wstr(fullUri));
			return;
		}
	}

	if (childrenIt != tile.end())
	{
		const json& children = *childrenIt;
		if (!children.is_array())
		{
			return;
		}

		for (auto& child : children)
		{
			AddTiles(actor, child, baseUrl);
		}
	}
}
