// Fill out your copyright notice in the Description page of Project Settings.


#include "CesiumGltf.h"
#include "tiny_gltf.h"
#include <locale>
#include <codecvt>
#include "StaticMeshDescription.h"
#include "GltfAccessor.h"

// temp
#define WIN32_LEAN_AND_MEAN
#include "windows.h"

FString utf8_to_wstr(const std::string& utf8) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> wcu8;
	return FString(wcu8.from_bytes(utf8).c_str());
}

std::string wstr_to_utf8(const FString& utf16) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> wcu8;
	return wcu8.to_bytes(*utf16);
}

// Sets default values
ACesiumGltf::ACesiumGltf() :
	Url(TEXT("C:\\Users\\kring\\Documents\\Cesium_Man.glb"))
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	this->RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Model"));
}

// Called when the game starts or when spawned
void ACesiumGltf::BeginPlay()
{
	Super::BeginPlay();

	tinygltf::TinyGLTF loader;
	tinygltf::Model model;
	std::string errors;
	std::string warnings;

	std::string url = wstr_to_utf8(this->Url);
	MessageBoxA(nullptr, url.c_str(), "Url", MB_OK);
	if (!loader.LoadBinaryFromFile(&model, &errors, &warnings, url))
	{
		MessageBoxA(nullptr, errors.c_str(), "Error", MB_OK);
		std::cout << errors << std::endl;
		return;
	}


	if (warnings.length() > 0)
	{
		MessageBoxA(nullptr, warnings.c_str(), "Warning", MB_OK);
	}


	UStaticMeshComponent* pMeshComponent = NewObject<UStaticMeshComponent>();
	pMeshComponent->AttachTo(this->RootComponent);

	UStaticMesh* pStaticMesh = NewObject<UStaticMesh>();
	pMeshComponent->SetStaticMesh(pStaticMesh);

	pStaticMesh->bIsBuiltAtRuntime = true;
	pStaticMesh->NeverStream = true;
	pStaticMesh->RenderData = MakeUnique<FStaticMeshRenderData>();
	pStaticMesh->RenderData->AllocateLODResources(1);

	FStaticMeshLODResources& LODResources = pStaticMesh->RenderData->LODResources[0];

	float centimetersPerMeter = 100.0f;

	for (auto meshIt = model.meshes.begin(); meshIt != model.meshes.end(); ++meshIt)
	{
		tinygltf::Mesh& mesh = *meshIt;

		for (auto primitiveIt = mesh.primitives.begin(); primitiveIt != mesh.primitives.end(); ++primitiveIt)
		{
			TMap<int32, FVertexID> indexToVertexIdMap;

			tinygltf::Primitive& primitive = *primitiveIt;
			auto positionAccessorIt = primitive.attributes.find("POSITION");
			if (positionAccessorIt == primitive.attributes.end()) {
				// This primitive doesn't have a POSITION semantic, ignore it.
				continue;
			}

			int positionAccessorID = positionAccessorIt->second;
			GltfAccessor<FVector> positionAccessor(model, positionAccessorID);

			TArray<FStaticMeshBuildVertex> StaticMeshBuildVertices;
			StaticMeshBuildVertices.SetNum(positionAccessor.size());

			for (size_t i = 0; i < positionAccessor.size(); ++i)
			{
				FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
				vertex.Position = positionAccessor[i] * centimetersPerMeter;
			}

			LODResources.VertexBuffers.PositionVertexBuffer.Init(StaticMeshBuildVertices);
			//LODResources.VertexBuffers.StaticMeshVertexBuffer.Init(StaticMeshBuildVertices, VertexInstanceUVs.GetNumIndices());

			// TODO: handle more than one primitive
			break;
		}

		// TODO: handle more than one mesh
		break;
	}

	pStaticMesh->InitResources();

	// Set up RenderData bounds and LOD data
	// TODO
	//pStaticMesh->RenderData->Bounds = MeshDescriptions[0]->GetBounds();
	pStaticMesh->CalculateExtendedBounds();

	pStaticMesh->RenderData->ScreenSize[0].Default = 1.0f;

	//TArray<const FMeshDescription*> meshDescriptions({&meshDescription});
	//pStaticMesh->BuildFromMeshDescriptions(meshDescriptions);
}

// Called every frame
void ACesiumGltf::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

