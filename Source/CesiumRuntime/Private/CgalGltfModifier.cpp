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

  CesiumGltf::Model modifiedModel = input.previousModel;

  UE_LOG(
      LogTemp,
      Log,
      TEXT("CgalGltfModifier::apply #%d — meshes=%d"),
      invocations,
      static_cast<int>(modifiedModel.meshes.size()));

  Cesium3DTilesSelection::GltfModifierOutput output;
  output.modifiedModel = std::move(modifiedModel);

  return input.asyncSystem
      .createResolvedFuture<
          std::optional<Cesium3DTilesSelection::GltfModifierOutput>>(
          std::move(output));
}
