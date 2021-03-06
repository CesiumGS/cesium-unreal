// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGltf.h"
#include "CesiumGltfComponent.h"
#include "SpdlogUnrealLoggerSink.h"

// Sets default values
ACesiumGltf::ACesiumGltf()
{
	this->RootComponent = this->Model = CreateDefaultSubobject<UCesiumGltfComponent>(TEXT("Model"));
}

// Called when the game starts or when spawned
void ACesiumGltf::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogCesium, VeryVerbose, TEXT("ACesiumGltf::BeginPlay"))
	this->Model->LoadModel(this->Url);
}

void ACesiumGltf::OnConstruction(const FTransform & Transform)
{
	UE_LOG(LogCesium, VeryVerbose, TEXT("ACesiumGltf::OnConstruction"))
	this->Model->LoadModel(this->Url);
}
