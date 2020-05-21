// Fill out your copyright notice in the Description page of Project Settings.


#include "Cesium3DTileset.h"
#include "json.hpp"
#include "tiny_gltf.h"
#include "UnrealStringConversions.h"
#include "CesiumGltfComponent.h"

void AddTiles(ACesium3DTileset& actor, const nlohmann::json& tile);

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
	
	std::vector<unsigned char> bytes;
	std::string errors;

	if (!tinygltf::ReadWholeFile(&bytes, &errors, wstr_to_utf8(this->Url), nullptr)) {
		return;
	}

	using nlohmann::json;
	json tileset = json::parse(bytes);

	json& root = tileset["root"];
	AddTiles(*this, root);
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

void AddTiles(ACesium3DTileset& actor, const nlohmann::json& tile)
{
	using nlohmann::json;

	if (!tile.is_object())
	{
		return;
	}

	json::const_iterator childrenIt = tile.find("children");
	json::const_iterator contentIt = tile.find("content");

	if (childrenIt != tile.end())
	{
		const json& children = *childrenIt;
		if (!children.is_array())
		{
			return;
		}

		for (auto& child : children)
		{
			AddTiles(actor, child);
		}
	}
	else if (contentIt != tile.end())
	{
		const json& content = *contentIt;
		const std::string& uri = content.value<std::string>("uri", "");

		// This is a leaf node, add it.
		UCesiumGltfComponent* Gltf = NewObject<UCesiumGltfComponent>(&actor);
		actor.AddGltf(Gltf);
		//Gltf->RegisterComponent();

		std::string fullUri = "C:\\cesium\\data\\agi-glb\\";
		fullUri += uri;

		Gltf->LoadModel(utf8_to_wstr(fullUri));
	}
}