// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Interfaces/IHttpRequest.h"
#include <glm/mat4x4.hpp>
#include <memory>
#include "CesiumGltfComponent.generated.h"

class UMaterial;
#if PHYSICS_INTERFACE_PHYSX
class IPhysXCooking;
#endif
class UTexture2D;

namespace CesiumGltf {
	struct Model;
}

namespace Cesium3DTiles {
	class Tile;
	class RasterOverlayTile;
}

namespace CesiumGeometry {
	struct Rectangle;
}

USTRUCT()
struct FRasterOverlayTile {
	GENERATED_BODY()
	UTexture2D* pTexture;
	FLinearColor textureCoordinateRectangle;
	FLinearColor translationAndScale;
};

UCLASS()
class CESIUM_API UCesiumGltfComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	class HalfConstructed {
	public:
		virtual ~HalfConstructed() = 0 {}
	};

	/// <summary>
	/// Constructs a UCesiumGltfComponent from the provided glTF model. This method does as much of the
	/// work in the calling thread as possible, and the calling thread need not be the game thread.
	/// The final component creation is done in the game thread (as required by Unreal Engine) and
	/// the provided callback is raised in the game thread with the result.
	/// </summary>
	static void CreateOffGameThread(AActor* pActor, const CesiumGltf::Model& model, const glm::dmat4x4& transform, TFunction<void (UCesiumGltfComponent*)>);
	static std::unique_ptr<HalfConstructed> CreateOffGameThread(
		const CesiumGltf::Model& model,
		const glm::dmat4x4& transform
#if PHYSICS_INTERFACE_PHYSX
		,IPhysXCooking* pPhysXCooking = nullptr
#endif
	);
	static UCesiumGltfComponent* CreateOnGameThread(
		AActor* pParentActor,
		std::unique_ptr<HalfConstructed> pHalfConstructed,
		const glm::dmat4x4& cesiumToUnrealTransform,
		UMaterial* pBaseMaterial
	);

	UCesiumGltfComponent();
	virtual ~UCesiumGltfComponent();

	UPROPERTY(EditAnywhere)
	UMaterial* BaseMaterial;

	UFUNCTION(BlueprintCallable)
	void LoadModel(const FString& Url);

	void UpdateTransformFromCesium(const glm::dmat4& cesiumToUnrealTransform);

	void AttachRasterTile(
		const Cesium3DTiles::Tile& tile,
		const Cesium3DTiles::RasterOverlayTile& rasterTile,
		UTexture2D* pTexture,
		const CesiumGeometry::Rectangle& textureCoordinateRectangle,
		const glm::dvec2& translation,
		const glm::dvec2& scale
	);

	void DetachRasterTile(
		const Cesium3DTiles::Tile& tile,
		const Cesium3DTiles::RasterOverlayTile& rasterTile,
		UTexture2D* pTexture,
		const CesiumGeometry::Rectangle& textureCoordinateRectangle
	);

	UFUNCTION(BlueprintCallable, Category = "Collision")
	virtual void SetCollisionEnabled(ECollisionEnabled::Type NewType);

	virtual void FinishDestroy() override;

protected:
	//virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	FString LoadedUrl;
	class UStaticMeshComponent* Mesh;

	glm::dmat4x4 _cesiumTransformation;

	TArray<FRasterOverlayTile> _overlayTiles;

private:
	void ModelRequestComplete(FHttpRequestPtr request, FHttpResponsePtr response, bool x);
	void updateRasterOverlays();
};
