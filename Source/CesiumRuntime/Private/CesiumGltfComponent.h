// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Components/SceneComponent.h"
#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include <glm/mat4x4.hpp>
#include <memory>
#include "CesiumGltfComponent.generated.h"

class UMaterial;
class UTexture2D;
class UStaticMeshComponent;

#if PHYSICS_INTERFACE_PHYSX
class IPhysXCooking;
#endif

namespace CesiumGltf {
struct Model;
}

namespace Cesium3DTiles {
class Tile;
class RasterOverlayTile;
} // namespace Cesium3DTiles

namespace CesiumGeometry {
struct Rectangle;
}

USTRUCT()
struct FRasterOverlayTile {
  GENERATED_BODY()

  UPROPERTY()
  UTexture2D* Texture;

  FLinearColor TextureCoordinateRectangle;
  FLinearColor TranslationAndScale;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumGltfComponent : public USceneComponent {
  GENERATED_BODY()

public:
  class HalfConstructed {
  public:
    virtual ~HalfConstructed() = default;
  };

  static std::unique_ptr<HalfConstructed> CreateOffGameThread(
      const CesiumGltf::Model& Model,
      const glm::dmat4x4& Transform
#if PHYSICS_INTERFACE_PHYSX
      ,
      IPhysXCooking* PhysXCooking = nullptr
#endif
  );

  static UCesiumGltfComponent* CreateOnGameThread(
      AActor* ParentActor,
      std::unique_ptr<HalfConstructed> HalfConstructed,
      const glm::dmat4x4& CesiumToUnrealTransform,
      UMaterial* BaseMaterial);

  UCesiumGltfComponent();
  virtual ~UCesiumGltfComponent();

  UPROPERTY(EditAnywhere, Category = "Cesium")
  UMaterial* BaseMaterial;

  UPROPERTY(EditAnywhere, Category = "Cesium")
  UMaterial* OpacityMaskMaterial;

  void UpdateTransformFromCesium(const glm::dmat4& CesiumToUnrealTransform);

  void AttachRasterTile(
      const Cesium3DTiles::Tile& Tile,
      const Cesium3DTiles::RasterOverlayTile& RasterTile,
      UTexture2D* Texture,
      const CesiumGeometry::Rectangle& TextureCoordinateRectangle,
      const glm::dvec2& Translation,
      const glm::dvec2& Scale);

  void DetachRasterTile(
      const Cesium3DTiles::Tile& Tile,
      const Cesium3DTiles::RasterOverlayTile& RasterTile,
      UTexture2D* Texture,
      const CesiumGeometry::Rectangle& TextureCoordinateRectangle);

  UFUNCTION(BlueprintCallable, Category = "Collision")
  virtual void SetCollisionEnabled(ECollisionEnabled::Type NewType);

  virtual void FinishDestroy() override;

private:
  void UpdateRasterOverlays();

  TArray<FRasterOverlayTile> OverlayTiles;
};
