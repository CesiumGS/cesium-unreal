// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "CoreMinimal.h"
#include "CustomDepthParameters.h"
#include "Interfaces/IHttpRequest.h"
#include <glm/mat4x4.hpp>
#include <memory>
#include "CesiumGltfComponent.generated.h"

class UMaterialInterface;
class UTexture2D;
class UStaticMeshComponent;
struct CreateModelOptions;

#if PHYSICS_INTERFACE_PHYSX
class IPhysXCooking;
#endif

namespace CesiumGltf {
struct Model;
}

namespace Cesium3DTilesSelection {
class Tile;
class RasterOverlayTile;
} // namespace Cesium3DTilesSelection

namespace CesiumGeometry {
struct Rectangle;
}

USTRUCT()
struct FRasterOverlayTile {
  GENERATED_BODY()

  UPROPERTY()
  FString OverlayName{};

  UPROPERTY()
  UTexture2D* Texture = nullptr;

  FLinearColor TranslationAndScale{};
  int32 TextureCoordinateID = -1;
};

UCLASS()
class UCesiumGltfComponent : public USceneComponent {
  GENERATED_BODY()

public:
  class HalfConstructed {
  public:
    virtual ~HalfConstructed() = default;
  };

  static std::unique_ptr<HalfConstructed> CreateOffGameThread(
      const CesiumGltf::Model& Model,
      const glm::dmat4x4& Transform,
      const CreateModelOptions& Options);

  static UCesiumGltfComponent* CreateOnGameThread(
      AActor* ParentActor,
      std::unique_ptr<HalfConstructed> HalfConstructed,
      const glm::dmat4x4& CesiumToUnrealTransform,
      UMaterialInterface* BaseMaterial,
      UMaterialInterface* BaseWaterMaterial,
      FCustomDepthParameters CustomDepthParameters);

  UCesiumGltfComponent();
  virtual ~UCesiumGltfComponent();

  UPROPERTY(EditAnywhere, Category = "Cesium")
  UMaterialInterface* BaseMaterial;

  UPROPERTY(EditAnywhere, Category = "Cesium")
  UMaterialInterface* BaseMaterialWithWater;

  UPROPERTY(EditAnywhere, Category = "Rendering")
  FCustomDepthParameters CustomDepthParameters;

  void UpdateTransformFromCesium(const glm::dmat4& CesiumToUnrealTransform);

  void AttachRasterTile(
      const Cesium3DTilesSelection::Tile& Tile,
      const Cesium3DTilesSelection::RasterOverlayTile& RasterTile,
      UTexture2D* Texture,
      const glm::dvec2& Translation,
      const glm::dvec2& Scale,
      int32_t TextureCoordinateID);

  void DetachRasterTile(
      const Cesium3DTilesSelection::Tile& Tile,
      const Cesium3DTilesSelection::RasterOverlayTile& RasterTile,
      UTexture2D* Texture);

  UFUNCTION(BlueprintCallable, Category = "Collision")
  virtual void SetCollisionEnabled(ECollisionEnabled::Type NewType);

private:
  UPROPERTY()
  UTexture2D* Transparent1x1;
};
