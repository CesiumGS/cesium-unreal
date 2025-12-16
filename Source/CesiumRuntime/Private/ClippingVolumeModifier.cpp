#include "ClippingVolumeModifier.h"
#include "Cesium3DTilesetRoot.h"
#include "VecMath.h"

#include <Cesium3DTilesSelection/GltfModifier.h>
#include <CesiumAsync/Future.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltfContent/GltfUtilities.h>
#include <unordered_set>

namespace {
template <typename IndexType>
std::vector<IndexType> pruneIndices(
    const CesiumGltf::AccessorView<IndexType>& indicesView,
    const std::unordered_set<size_t>& removed,
    int32_t primitiveMode) {
  std::vector<IndexType> result;
  result.reserve(indicesView.size());

  switch (primitiveMode) {
  case CesiumGltf::MeshPrimitive::Mode::TRIANGLES:
    for (int64_t i = 0; i < indicesView.size(); i += 3) {
      if (i + 1 >= indicesView.size() || i + 2 >= indicesView.size()) {
        return std::vector<IndexType>();
      }

      if (removed.contains(indicesView[i]) ||
          removed.contains(indicesView[i + 1]) ||
          removed.contains(indicesView[i + 2])) {
        continue;
      }

      result.push_back(indicesView[i]);
      result.push_back(indicesView[i + 1]);
      result.push_back(indicesView[i + 2]);
    }
    break;
  }

  result.shrink_to_fit();
  return result;
}
} // namespace

CesiumAsync::Future<std::optional<Cesium3DTilesSelection::GltfModifierOutput>>
ClippingVolumeModifier::apply(
    Cesium3DTilesSelection::GltfModifierInput&& input) {
  Cesium3DTilesSelection::GltfModifierOutput output;
  output.modifiedModel = input.previousModel;

  if (!this->_pTileset.IsValid()) {
    return input.asyncSystem.createResolvedFuture(
        std::make_optional(std::move(output)));
  }

  UCesium3DTilesetRoot* pRoot =
      Cast<UCesium3DTilesetRoot>(this->_pTileset->GetRootComponent());
  if (!pRoot) {
    return input.asyncSystem.createResolvedFuture(
        std::make_optional(std::move(output)));
  }

  const glm::dmat4& tilesetToUnreal =
      pRoot->GetCesiumTilesetToUnrealRelativeWorldTransform();
  FMatrix unrealToTile =
      VecMath::createMatrix(tilesetToUnreal * input.tileTransform).Inverse();

  const TArray<TSoftObjectPtr<ATriggerBox>>& clippingVolumes =
      this->_pTileset->GetClippingVolumes();

  output.modifiedModel.forEachPrimitiveInScene(
      output.modifiedModel.scene,
      [&model = output.modifiedModel, &unrealToTile, &clippingVolumes](
          const CesiumGltf::Model& gltf,
          const CesiumGltf::Node& node,
          const CesiumGltf::Mesh& mesh,
          const CesiumGltf::MeshPrimitive& primitive,
          const glm::dmat4& transform) {
        FMatrix tileToPrimitive = VecMath::createMatrix(transform).Inverse();

        for (const TSoftObjectPtr<ATriggerBox>& pTriggerBox : clippingVolumes) {
          if (!pTriggerBox) {
            continue;
          }

          CesiumGltf::Accessor* pPositionAccessor = nullptr;
          CesiumGltf::BufferView* pPositionBufferView = nullptr;
          CesiumGltf::Buffer* pPositionBuffer = nullptr;

          auto positionIt = primitive.attributes.find("POSITION");
          if (positionIt != primitive.attributes.end()) {
            pPositionAccessor = CesiumGltf::Model::getSafe(
                &model.accessors,
                positionIt->second);
          }

          if (pPositionAccessor) {
            pPositionBufferView = CesiumGltf::Model::getSafe(
                &model.bufferViews,
                pPositionAccessor->bufferView);
          } else {
            return;
          }

          if (pPositionBufferView) {
            pPositionBuffer = CesiumGltf::Model::getSafe(
                &model.buffers,
                pPositionBufferView->buffer);
          } else {
            return;
          }

          CesiumGltf::AccessorView<glm::vec3> positionView(
              gltf,
              *pPositionAccessor);
          if (positionView.status() != CesiumGltf::AccessorViewStatus::Valid) {
            return;
          }

          if (pPositionAccessor->min.size() != 3 ||
              pPositionAccessor->max.size() != 3) {
            return;
          }

          FBox primitiveBounds(
              {FVector(
                   pPositionAccessor->min[0],
                   pPositionAccessor->min[1],
                   pPositionAccessor->min[2]),
               FVector(
                   pPositionAccessor->max[0],
                   pPositionAccessor->max[1],
                   pPositionAccessor->max[2])});

          FBox box = pTriggerBox->GetComponentsBoundingBox();
          FBox transformedClippingVolume =
              box.TransformBy(unrealToTile).TransformBy(tileToPrimitive);

          if (!primitiveBounds.Intersect(transformedClippingVolume)) {
            return;
          }

          int64_t bufferIndex = int64_t(model.buffers.size());
          CesiumGltf::Accessor* pIndexAccessor =
              CesiumGltf::Model::getSafe(&model.accessors, primitive.indices);

          if (pIndexAccessor) {
            CesiumGltf::BufferView* pIndexBufferView =
                CesiumGltf::Model::getSafe(
                    &model.bufferViews,
                    pIndexAccessor->bufferView);
            if (!pIndexBufferView)
              return;

            CesiumGltf::Buffer* pIndexBuffer = CesiumGltf::Model::getSafe(
                &model.buffers,
                pIndexBufferView->buffer);

            // Identify which indices to remove.
            std::unordered_set<size_t> removedIndices;
            for (int64_t i = 0; i < positionView.size(); i++) {
              if (transformedClippingVolume.ComputeSquaredDistanceToPoint(
                      FVector(
                          positionView[i][0],
                          positionView[i][1],
                          positionView[i][2]))) {
                removedIndices.insert(size_t(i));
              }
            }

            if (removedIndices.empty()) {
              // nothing clipped, don't proceed.
              return;
            }

            CesiumGltf::Buffer& buffer = model.buffers.emplace_back();
            size_t newCount = 0;

            if (pIndexAccessor->componentType ==
                CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE) {
              std::vector<uint8_t> newIndices = pruneIndices(
                  CesiumGltf::AccessorView<uint8_t>(gltf, *pIndexAccessor),
                  removedIndices,
                  primitive.mode);
              newCount = newIndices.size();
              buffer.byteLength = sizeof(uint8_t) * newIndices.size();
              buffer.cesium.data.resize(buffer.byteLength);
              std::memcpy(
                  buffer.cesium.data.data(),
                  newIndices.data(),
                  newIndices.size());
            } else if (
                pIndexAccessor->componentType ==
                CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT) {
              std::vector<uint16_t> newIndices = pruneIndices(
                  CesiumGltf::AccessorView<uint16_t>(gltf, *pIndexAccessor),
                  removedIndices,
                  primitive.mode);
              newCount = newIndices.size();
              buffer.byteLength = sizeof(uint16_t) * newIndices.size();
              buffer.cesium.data.resize(buffer.byteLength);
              std::memcpy(
                  buffer.cesium.data.data(),
                  newIndices.data(),
                  newIndices.size());
            } else if (
                pIndexAccessor->componentType ==
                CesiumGltf::Accessor::ComponentType::UNSIGNED_INT) {
              std::vector<uint32_t> newIndices = pruneIndices(
                  CesiumGltf::AccessorView<uint32_t>(gltf, *pIndexAccessor),
                  removedIndices,
                  primitive.mode);
              newCount = newIndices.size();
              buffer.byteLength = sizeof(uint32_t) * newIndices.size();
              buffer.cesium.data.resize(buffer.byteLength);
              std::memcpy(
                  buffer.cesium.data.data(),
                  newIndices.data(),
                  newIndices.size());
            } else {
              std::vector<uint64_t> newIndices = pruneIndices(
                  CesiumGltf::AccessorView<uint64_t>(gltf, *pIndexAccessor),
                  removedIndices,
                  primitive.mode);
              newCount = newIndices.size();
              buffer.byteLength = sizeof(uint64_t) * newIndices.size();
              buffer.cesium.data.resize(buffer.byteLength);
              std::memcpy(
                  buffer.cesium.data.data(),
                  newIndices.data(),
                  newIndices.size());
            }

            pIndexBufferView->buffer = bufferIndex;
            pIndexBufferView->byteOffset = 0;
            pIndexBufferView->byteLength = buffer.byteLength;
            pIndexBufferView->byteStride = std::nullopt;
            pIndexAccessor->count = newCount;
          } else {
            // Prune positions directly
            std::vector<glm::vec3> newPositions;
            newPositions.reserve(positionView.size());
            for (int64_t i = 0; i < positionView.size(); i++) {
              if (transformedClippingVolume.ComputeSquaredDistanceToPoint(
                      FVector(
                          positionView[i][0],
                          positionView[i][1],
                          positionView[i][2]))) {
                newPositions.emplace_back(positionView[i]);
              }
            }

            CesiumGltf::Buffer& buffer = model.buffers.emplace_back();
            buffer.byteLength = sizeof(glm::vec3) * newPositions.size();
            buffer.cesium.data.resize(buffer.byteLength);
            std::memcpy(
                buffer.cesium.data.data(),
                newPositions.data(),
                newPositions.size());
            pPositionBufferView->buffer = bufferIndex;
            pPositionBufferView->byteOffset = 0;
            pPositionBufferView->byteLength = buffer.byteLength;
            pPositionBufferView->byteStride = std::nullopt;
            pPositionAccessor->count = newPositions.size();
          }
        }
      });

  return input.asyncSystem.createResolvedFuture(
      std::make_optional(std::move(output)));
}
