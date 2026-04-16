#pragma once

#include <Cesium3DTilesSelection/GltfModifier.h>
#include <CesiumAsync/Future.h>

#include <array>
#include <memory>
#include <optional>
#include <vector>

#include <glm/glm.hpp>

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

  /**
   * Extract the StaticMeshActor's mesh data and transform vertices to ECEF.
   * Must be called on the game thread before apply() will perform clipping.
   */
  void CacheClipMeshData(const glm::dmat4& unrealToEcef);

protected:
  CesiumAsync::Future<void> onRegister(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const Cesium3DTilesSelection::TilesetMetadata& tilesetMetadata,
      const Cesium3DTilesSelection::Tile& rootTile) override;

  AStaticMeshActor* StaticMeshActor = nullptr;

  // Clip mesh data cached on the game thread for use in apply().
  std::vector<std::array<float, 3>> ClipMeshPositionsECEF;
  std::vector<uint32_t> ClipMeshIndices;
  bool bClipMeshReady = false;
};
