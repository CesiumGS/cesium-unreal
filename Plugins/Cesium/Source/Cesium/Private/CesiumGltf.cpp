// Fill out your copyright notice in the Description page of Project Settings.


#include "CesiumGltf.h"
#include "tiny_gltf.h"
#include <locale>
#include <codecvt>
#include "StaticMeshDescription.h"

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
ACesiumGltf::ACesiumGltf()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

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

	this->Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	this->Mesh->SetupAttachment(this->RootComponent);

	UStaticMesh* pStaticMesh = NewObject<UStaticMesh>();
	this->Mesh->SetStaticMesh(pStaticMesh);

	// TODO: In theory we can load the glTF data directly into the UStaticMesh by
	// imitating what UStaticMesh::BuildFromMeshDescriptions does, and that would
	// save us a copy.
	FMeshDescription meshDescription;
	FStaticMeshAttributes(meshDescription).Register();
	FStaticMeshAttributes StaticMeshAttributes(meshDescription);
	TVertexAttributesRef<FVector> VertexPositions = StaticMeshAttributes.GetVertexPositions();

	TArray<const FMeshDescription*> meshDescriptions({&meshDescription});
	pStaticMesh->BuildFromMeshDescriptions(meshDescriptions);
}

// Called every frame
void ACesiumGltf::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

