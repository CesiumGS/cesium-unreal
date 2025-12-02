// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Cesium3DTilesSelection/Tile.h"
#include "Cesium3DTileset.h"
#include "CesiumEncodedMetadataUtility.h"
#include "CesiumLoadedTile.h"
#include "CesiumModelMetadata.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "CoreMinimal.h"
#include "CustomDepthParameters.h"
#include "EncodedFeaturesMetadata.h"
#include "Interfaces/IHttpRequest.h"
#include "Templates/Function.h"
#include <CesiumAsync/SharedFuture.h>
#include <glm/mat4x4.hpp>
#include <memory>
#include "CesiumGltfComponent.generated.h"

class UMaterialInterface;
class UTexture2D;
class UStaticMeshComponent;

namespace CreateGltfOptions {
struct CreateModelOptions;
}

namespace CesiumGltf {
struct Model;
}

namespace Cesium3DTilesSelection {
class Tile;
}

namespace CesiumRasterOverlays {
class RasterOverlayTile;
}

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
class UCesiumGltfComponent : public USceneComponent, public ICesiumLoadedTile {
  GENERATED_BODY()

public:
  class HalfConstructed {
  public:
    virtual ~HalfConstructed() = default;
  };

  class CreateOffGameThreadResult {
  public:
    TUniquePtr<UCesiumGltfComponent::HalfConstructed> HalfConstructed;
    Cesium3DTilesSelection::TileLoadResult TileLoadResult;
  };

  static CesiumAsync::Future<CreateOffGameThreadResult> CreateOffGameThread(
      const CesiumAsync::AsyncSystem& AsyncSystem,
      const glm::dmat4x4& Transform,
      CreateGltfOptions::CreateModelOptions&& Options,
      const CesiumGeospatial::Ellipsoid& Ellipsoid =
          CesiumGeospatial::Ellipsoid::WGS84);

  static UCesiumGltfComponent* CreateOnGameThread(
      CesiumGltf::Model& model,
      ACesium3DTileset* ParentActor,
      TUniquePtr<HalfConstructed> HalfConstructed,
      const glm::dmat4x4& CesiumToUnrealTransform,
      UMaterialInterface* BaseMaterial,
      UMaterialInterface* BaseTranslucentMaterial,
      UMaterialInterface* BaseWaterMaterial,
      FCustomDepthParameters CustomDepthParameters,
      Cesium3DTilesSelection::Tile& tile,
      bool createNavCollision);

  UCesiumGltfComponent();

  UPROPERTY(EditAnywhere, Category = "Cesium")
  UMaterialInterface* BaseMaterial = nullptr;

  UPROPERTY(EditAnywhere, Category = "Cesium")
  UMaterialInterface* BaseMaterialWithTranslucency = nullptr;

  UPROPERTY(EditAnywhere, Category = "Cesium")
  UMaterialInterface* BaseMaterialWithWater = nullptr;

  UPROPERTY(EditAnywhere, Category = "Rendering")
  FCustomDepthParameters CustomDepthParameters{};

  const Cesium3DTilesSelection::Tile* pTile = nullptr;

  FCesiumModelMetadata Metadata{};
  EncodedFeaturesMetadata::EncodedModelMetadata EncodedMetadata{};

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  std::optional<CesiumEncodedMetadataUtility::EncodedMetadata>
      EncodedMetadata_DEPRECATED = std::nullopt;
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  void UpdateTransformFromCesium(const glm::dmat4& CesiumToUnrealTransform);

  void AttachRasterTile(
      const Cesium3DTilesSelection::Tile& Tile,
      const CesiumRasterOverlays::RasterOverlayTile& RasterTile,
      UTexture2D* Texture,
      const glm::dvec2& Translation,
      const glm::dvec2& Scale,
      int32_t TextureCoordinateID);

  void DetachRasterTile(
      const Cesium3DTilesSelection::Tile& Tile,
      const CesiumRasterOverlays::RasterOverlayTile& RasterTile,
      UTexture2D* Texture);

  UFUNCTION(BlueprintCallable, Category = "Collision")
  virtual void SetCollisionEnabled(ECollisionEnabled::Type NewType);

  virtual void BeginDestroy() override;
  virtual void OnVisibilityChanged() override;

  // from ICesiumLoadedTile
  const CesiumGltf::Model* GetGltfModel() const override;
  const FCesiumModelMetadata& GetModelMetadata() const override;
  const Cesium3DTilesSelection::TileID& GetTileID() const override;
  ACesium3DTileset& GetTilesetActor() override;
  FVector GetGltfToUnrealLocalVertexPositionScaleFactor() const override;

  void UpdateFade(float fadePercentage, bool fadingIn);

private:
  UPROPERTY()
  UTexture2D* Transparent1x1 = nullptr;
};
