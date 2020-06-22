// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IHttpRequest.h"
#include "ACesium3DTileset.generated.h"

namespace Cesium3DTiles {
	class Tileset;
	class TilesetView;
}

UCLASS()
class CESIUM_API ACesium3DTileset : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACesium3DTileset();

	UPROPERTY(EditAnywhere, Category = "Cesium")
	FString Url;

	UPROPERTY(EditAnywhere, Category = "Cesium")
	uint32 IonAssetID;

	UPROPERTY(EditAnywhere, Category = "Cesium")
	FString IonAccessToken;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	void IonAssetRequestComplete(FHttpRequestPtr request, FHttpResponsePtr response, bool x);
	void TilesetJsonRequestComplete(FHttpRequestPtr request, FHttpResponsePtr response, bool x);
	void LoadTileset();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	// TODO: this shouldn't be public.
	void AddGltf(class UCesiumGltfComponent* Gltf);

private:
	Cesium3DTiles::Tileset* _pTileset;
	Cesium3DTiles::TilesetView* _pTilesetView;
};
