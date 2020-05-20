// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CesiumGltf.generated.h"

UCLASS()
class CESIUM_API ACesiumGltf : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACesiumGltf();

	void OnConstruction(const FTransform & Transform);

	UPROPERTY(EditAnywhere, Category = "Cesium")
	FString Url;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	class UCesiumGltfComponent* Model;
};
