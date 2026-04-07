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

  const auto& model = input.previousModel;
  const auto& t = input.tileTransform;

  int64_t vertexCount = 0;
  int64_t indexCount = 0;
  int numAttributes = 0;
  if (!model.meshes.empty() && !model.meshes[0].primitives.empty()) {
    const auto& prim = model.meshes[0].primitives[0];
    numAttributes = static_cast<int>(prim.attributes.size());
    auto posIt = prim.attributes.find("POSITION");
    if (posIt != prim.attributes.end() && posIt->second >= 0 &&
        posIt->second < static_cast<int32_t>(model.accessors.size())) {
      vertexCount = model.accessors[posIt->second].count;
    }
    if (prim.indices >= 0 &&
        prim.indices < static_cast<int32_t>(model.accessors.size())) {
      indexCount = model.accessors[prim.indices].count;
    }
  }

  UE_LOG(
      LogTemp,
      Log,
      TEXT("CgalGltfModifier::apply — version=%lld, vertices=%lld, "
           "indices=%lld, attributes=%d, accessors=%d, buffers=%d, "
           "bufferViews=%d, tilePos=(%.2f, %.2f, %.2f)"),
      static_cast<long long>(input.version),
      static_cast<long long>(vertexCount),
      static_cast<long long>(indexCount),
      numAttributes,
      static_cast<int>(model.accessors.size()),
      static_cast<int>(model.buffers.size()),
      static_cast<int>(model.bufferViews.size()),
      t[3][0], t[3][1], t[3][2]);

  CesiumGltf::Model modifiedModel = input.previousModel;

  Cesium3DTilesSelection::GltfModifierOutput output;
  output.modifiedModel = std::move(modifiedModel);

  return input.asyncSystem
      .createResolvedFuture<std::optional<Cesium3DTilesSelection::GltfModifierOutput>>(
          std::move(output));
}
