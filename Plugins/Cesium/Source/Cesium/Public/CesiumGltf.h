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

	UPROPERTY(EditAnywhere, Category = "Cesium")
	FString Url;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform);

	class UCesiumGltfComponent* Model;
};
