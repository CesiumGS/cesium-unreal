// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Interfaces/IHttpRequest.h"
#include "CesiumGltfComponent.generated.h"

class UMaterial;

namespace tinygltf{
	class Model;
}

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CESIUM_API UCesiumGltfComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	UCesiumGltfComponent();

	UFUNCTION(BlueprintCallable)
	void LoadModel(const FString& Url);

	void LoadModel(const tinygltf::Model& model);

protected:
	//virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere)
	UMaterial* BaseMaterial;

	FString LoadedUrl;
	class UStaticMeshComponent* Mesh;

private:
	void ModelRequestComplete(FHttpRequestPtr request, FHttpResponsePtr response, bool x);
};
