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
	// Constructs a UCesiumGltfComponent from the provided glTF model. This method does as much of the
	// work in the calling thread as possible, and the calling thread need not be the game thread.
	// The final component creation is done in the game thread (as required by Unreal Engine) and
	// the provided callback is raised in the game thread with the result.
	static void CreateOffGameThread(AActor* pActor, const tinygltf::Model& model, TFunction<void (UCesiumGltfComponent*)>);

	UCesiumGltfComponent();

	UPROPERTY(EditAnywhere)
	UMaterial* BaseMaterial;

	UFUNCTION(BlueprintCallable)
	void LoadModel(const FString& Url);

	void LoadModel(const tinygltf::Model& model);

protected:
	//virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


	FString LoadedUrl;
	class UStaticMeshComponent* Mesh;

private:
	void ModelRequestComplete(FHttpRequestPtr request, FHttpResponsePtr response, bool x);
};
