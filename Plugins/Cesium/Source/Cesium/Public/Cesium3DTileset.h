// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Cesium3DTileset.generated.h"

UCLASS()
class CESIUM_API ACesium3DTileset : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACesium3DTileset();

	UPROPERTY(EditAnywhere, Category = "Cesium")
	FString Url;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	// TODO: this shouldn't be public.
	void AddGltf(class UCesiumGltfComponent* Gltf);
};
