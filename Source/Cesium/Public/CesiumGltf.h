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

	UPROPERTY(EditAnywhere, Category="Cesium")
	FString Url;

	UPROPERTY()
	UMaterial* BaseMaterial;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform & Transform);
};
