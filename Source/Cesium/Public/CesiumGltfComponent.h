// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CesiumGltfComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CESIUM_API UCesiumGltfComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	UCesiumGltfComponent();

	UFUNCTION(BlueprintCallable)
	void LoadModel(const FString& Url);

protected:
	UPROPERTY(EditAnywhere)
	UMaterial* BaseMaterial;

	FString LoadedUrl;
	class UStaticMeshComponent* Mesh;
};
