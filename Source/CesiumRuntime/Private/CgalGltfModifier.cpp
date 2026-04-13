#include "CgalGltfModifier.h"

#include <Cesium3DTilesSelection/TilesetMetadata.h>
#include <CesiumGltf/Model.h>

#include "CgalMeshOperations.h"

CgalGltfModifier::CgalGltfModifier() = default;
CgalGltfModifier::~CgalGltfModifier() = default;

void CgalGltfModifier::SetStaticMeshActor(AStaticMeshActor* InStaticMeshActor) {
  StaticMeshActor = InStaticMeshActor;
}

AStaticMeshActor* CgalGltfModifier::GetStaticMeshActor() const {
  return StaticMeshActor;
}

CesiumAsync::Future<void> CgalGltfModifier::onRegister(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const Cesium3DTilesSelection::TilesetMetadata& tilesetMetadata,
    const Cesium3DTilesSelection::Tile& rootTile) {
  return asyncSystem.createResolvedFuture();
}


CesiumAsync::Future<std::optional<Cesium3DTilesSelection::GltfModifierOutput>>
CgalGltfModifier::apply(
    Cesium3DTilesSelection::GltfModifierInput&& input) {

  static int invocations = 0;
  ++invocations;

  auto countTriangles = [](const CesiumGltf::Model& model) {
    int64_t total = 0;
    for (const auto& mesh : model.meshes) {
      for (const auto& prim : mesh.primitives) {
        if (prim.mode != CesiumGltf::MeshPrimitive::Mode::TRIANGLES)
          continue;
        if (prim.indices >= 0 &&
            prim.indices < static_cast<int32_t>(model.accessors.size())) {
          total += model.accessors[prim.indices].count / 3;
        }
      }
    }
    return total;
  };

  int64_t inputTriangles = countTriangles(input.previousModel);

  CesiumGltf::Model modifiedModel = input.previousModel;

  // TODO: actual modification logic goes here

  int64_t outputTriangles = countTriangles(modifiedModel);

  UE_LOG(
      LogTemp,
      Log,
      TEXT("CgalGltfModifier::apply #%d — input triangles=%lld, "
           "output triangles=%lld"),
      invocations,
      static_cast<long long>(inputTriangles),
      static_cast<long long>(outputTriangles));

  Cesium3DTilesSelection::GltfModifierOutput output;
  output.modifiedModel = std::move(modifiedModel);

  return input.asyncSystem
      .createResolvedFuture<
          std::optional<Cesium3DTilesSelection::GltfModifierOutput>>(
          std::move(output));
}
