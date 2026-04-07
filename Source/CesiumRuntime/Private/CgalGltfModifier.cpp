#include "CgalGltfModifier.h"

#include <Cesium3DTilesSelection/TilesetMetadata.h>
#include <CesiumGltf/Model.h>

#include "CgalMeshOperations.h"

CgalGltfModifier::CgalGltfModifier() = default;
CgalGltfModifier::~CgalGltfModifier() = default;

CesiumAsync::Future<void> CgalGltfModifier::onRegister(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const Cesium3DTilesSelection::TilesetMetadata& tilesetMetadata,
    const Cesium3DTilesSelection::Tile& rootTile) {
  // Perform any one-time initialization here (e.g. loading CGAL data).
  return asyncSystem.createResolvedFuture();
}

CesiumAsync::Future<std::optional<Cesium3DTilesSelection::GltfModifierOutput>>
CgalGltfModifier::apply(
    Cesium3DTilesSelection::GltfModifierInput&& input) {
  // TODO: Implement CGAL-based mesh modification here.
  // For now, make a copy of the model and return it unmodified.
  // Use CesiumCgal::ClipMesh() from CgalMeshOperations.h to perform clipping.
  CesiumGltf::Model modifiedModel = input.previousModel;

  Cesium3DTilesSelection::GltfModifierOutput output;
  output.modifiedModel = std::move(modifiedModel);

  return input.asyncSystem
      .createResolvedFuture<std::optional<Cesium3DTilesSelection::GltfModifierOutput>>(
          std::move(output));
}
