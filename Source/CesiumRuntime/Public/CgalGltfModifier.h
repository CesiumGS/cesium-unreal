#pragma once

#include <Cesium3DTilesSelection/GltfModifier.h>
#include <CesiumAsync/Future.h>

#include <memory>
#include <optional>

class AStaticMeshActor;

/**
 * A custom GltfModifier that uses CGAL to modify tile glTF models.
 *
 * Subclass this and override apply() to implement your CGAL-based
 * mesh processing logic.
 */
class CgalGltfModifier : public Cesium3DTilesSelection::GltfModifier {
public:
  CgalGltfModifier();
  virtual ~CgalGltfModifier() override;

  CesiumAsync::Future<std::optional<Cesium3DTilesSelection::GltfModifierOutput>>
  apply(Cesium3DTilesSelection::GltfModifierInput&& input) override;

  void SetStaticMeshActor(AStaticMeshActor* InStaticMeshActor);
  AStaticMeshActor* GetStaticMeshActor() const;

protected:
  CesiumAsync::Future<void> onRegister(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const Cesium3DTilesSelection::TilesetMetadata& tilesetMetadata,
      const Cesium3DTilesSelection::Tile& rootTile) override;

  AStaticMeshActor* StaticMeshActor = nullptr;
};
