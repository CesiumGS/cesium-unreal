// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGltfPrimitiveEdges.h"
#include "CesiumRuntime.h"
#include "CesiumTextureResource.h"
#include "CesiumTextureUtility.h"
#include "ExtensionImageAssetUnreal.h"
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/ExtensionExtMeshPrimitiveEdgeVisibility.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/VertexAttributeSemantics.h>
#include <CesiumGltfReader/GltfReader.h>

using namespace CesiumGltf;

namespace {
template <typename TIndex>
TUniquePtr<FStaticMeshRenderData> createInWorkerThreadImpl(
    const CesiumGltf::Model& model,
    const CesiumGltf::AccessorView<FVector3f>& positionView,
    const CesiumGltf::AccessorView<TIndex>& indicesView,
    const CesiumGltf::ExtensionExtMeshPrimitiveEdgeVisibility& extension);
}

/*static*/ TUniquePtr<FStaticMeshRenderData>
CesiumGltfPrimitiveEdges::createInWorkerThread(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    const CesiumGltf::AccessorView<FVector3f>& positionView,
    const CesiumGltf::ExtensionExtMeshPrimitiveEdgeVisibility& extension) {
  if (positionView.status() != CesiumGltf::AccessorViewStatus::Valid) {
    return nullptr;
  }

  const CesiumGltf::Accessor* pIndexAccessor =
      Model::getSafe(&model.accessors, primitive.indices);
  if (!pIndexAccessor) {
    return nullptr;
  }

  // Remake the index accessor view here to avoid templates in the .h file.
  switch (pIndexAccessor->componentType) {
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE: {
    CesiumGltf::AccessorView<uint8_t> indicesView(model, *pIndexAccessor);
    return createInWorkerThreadImpl(
        model,
        positionView,
        indicesView,
        extension);
  }
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT: {
    CesiumGltf::AccessorView<uint16_t> indicesView(model, *pIndexAccessor);
    return createInWorkerThreadImpl(
        model,
        positionView,
        indicesView,
        extension);
  }
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_INT: {
    CesiumGltf::AccessorView<uint32_t> indicesView(model, *pIndexAccessor);
    return createInWorkerThreadImpl(
        model,
        positionView,
        indicesView,
        extension);
  }
  default:
    return nullptr;
  }
}

namespace {

/**
 * Edge data extracted from EXT_mesh_primitive_edge_visibility.
 */
struct VisibleEdgeResult {
  /**
   * The indices
   */
  std::vector<uint32_t> edgeIndices;
  /**
   * The types of each edge.
   */
  std::vector<uint8_t> edgeTypes;
  std::vector<int64_t> silhouetteNormalIndices;
};

/**
 * @brief Parses the `visibility` stream from EXT_mesh_primitive_edge_visibility
 * and extracts a unique set of edges to render.
 *
 * @param visibility The view of the `visibility` accessor used by the
 * extension.
 * @param indices The view of the `indices` accessor used by the primitive.
 * @param vertexCount The number of vertices in the primitive.
 */
template <typename TIndex>
VisibleEdgeResult extractVisibleEdges(
    const CesiumGltf::AccessorView<uint8_t>& visibility,
    const CesiumGltf::AccessorView<TIndex>& indices,
    int64_t vertexCount) {
  VisibleEdgeResult result;

  int64_t edgeIndex = 0;
  int32_t silhouetteEdgeCount = 0;
  TSet<uint32> seenEdgeHashes;

  for (int64_t indexOffset = 0; indexOffset + 2 < indices.size();
       indexOffset += 3) {
    // Iterate over triangles (every three indices).
    for (uint8_t vertex = 0; vertex < 3; vertex++) {
      // Identify the next edge in the triangle.
      // For each triangle (v0, v1, v2), the bitfield encodes three visibility
      // values for the edges (v0:v1, v1:v2, v2:v0) in that order.
      uint8_t nextVertex = (vertex + 1) % 3;
      TIndex a = indices[indexOffset + vertex];
      TIndex b = indices[indexOffset + nextVertex];

      // Get the corresponding visibility value for the edge.
      uint32 byteIndex = FMath::Floor(edgeIndex / 4);
      uint32 bitPairOffset = (edgeIndex % 4) * 2;
      edgeIndex++;
      if (byteIndex >= visibility.size()) {
        break;
      }

      // Get the edge type encoded as two bits.
      uint8_t byte = visibility[byteIndex];
      uint8_t edgeType = (byte >> bitPairOffset) & 0x3;

      if (edgeType !=
          ExtensionExtMeshPrimitiveEdgeVisibility::Visibility::HIDDEN) {
        const uint32 smallestIndex = FMath::Min(a, b);
        const uint32 biggestIndex = FMath::Max(a, b);
        const uint32 hash = smallestIndex * vertexCount + biggestIndex;

        // Create the edge if it hasn't already been generated.
        if (!seenEdgeHashes.Contains(hash)) {
          seenEdgeHashes.Add(hash);
          result.edgeTypes.emplace_back(edgeType);
          result.edgeIndices.emplace_back(uint32_t(a));
          result.edgeIndices.emplace_back(uint32_t(b));

          if (edgeType ==
              ExtensionExtMeshPrimitiveEdgeVisibility::Visibility::SILHOUETTE) {
            result.silhouetteNormalIndices.emplace_back(silhouetteEdgeCount);
            silhouetteEdgeCount++;
          } else {
            result.silhouetteNormalIndices.emplace_back(-1);
          }
        }
      }
    }
  }
  return result;
}

template <typename TNormal>
void populateSilhouetteNormals(
    const VisibleEdgeResult& visibleEdgeResult,
    const CesiumGltf::AccessorView<TNormal>& silhouetteNormals,
    FStaticMeshVertexBuffer& edgeVertexBuffer) {
  // Add silhouette normals for silhouette edges (type 1)
  // tangentX = faceNormalA
  // tangentZ = faceNormalB
  if (edgeType ==
          ExtensionExtMeshPrimitiveEdgeVisibility::Visibility::SILHOUETTE &&
      hasSilhouetteNormals) {
    uint32 mateVertexIndex =
        visibleEdges.edgeData[e].mateVertexIndex.value_or(0);
    if (mateVertexIndex < silhouetteNormalAccessor.size()) {
      // silhouetteNormal =
      // silhouetteNormalAccessor[mateVertexIndex];
    }
  }
  FVector3f faceNormalA(0, 0, 1);
  FVector3f faceNormalB(0, 0, 1);

  edgeVertexBuffer
      .SetVertexTangents(e, silhouetteNormal, faceNormalA, faceNormalB);
  edgeVertexBuffer
      .SetVertexTangents(e + 1, silhouetteNormal, faceNormalA, faceNormalB);
}

template <typename TIndex>
TUniquePtr<FStaticMeshRenderData> createInWorkerThreadImpl(
    const CesiumGltf::Model& model,
    const CesiumGltf::AccessorView<FVector3f>& positionView,
    const CesiumGltf::AccessorView<TIndex>& indicesView,
    const CesiumGltf::ExtensionExtMeshPrimitiveEdgeVisibility& extension) {
  CesiumGltf::AccessorView<uint8_t> visibilityView(model, extension.visibility);
  if (visibilityView.status() != CesiumGltf::AccessorViewStatus::Valid) {
    // TODO: LOG
    return nullptr;
  }

  VisibleEdgeResult visibleEdges =
      extractVisibleEdges(visibilityView, indicesView, positionView.size());
  if (visibleEdges.edgeTypes.empty()) {
    // No edges were detected.
    return nullptr;
  }

  TUniquePtr<FStaticMeshRenderData> pRenderData =
      MakeUnique<FStaticMeshRenderData>();
  pRenderData->AllocateLODResources(1);

  FStaticMeshLODResources& lodResources = pRenderData->LODResources[0];

  size_t edgeCount = visibleEdges.edgeTypes.size();
  size_t totalEdgeVertices = edgeCount * 2;

  FPositionVertexBuffer& edgePositions =
      lodResources.VertexBuffers.PositionVertexBuffer;
  edgePositions.Init(totalEdgeVertices, false);

  // Edge type is passed through as a UV coordinate in the edge vertex buffer.
  FStaticMeshVertexBuffer& edgeVertexBuffer =
      lodResources.VertexBuffers.StaticMeshVertexBuffer;
  edgeVertexBuffer.Init(totalEdgeVertices, 1, false);

  TIndex vertexCount = TIndex(positionView.size());
  for (size_t i = 0; i < edgeCount; i++) {
    uint32 edgeVertexIndexA = i * 2;
    uint32 edgeVertexIndexB = i * 2 + 1;

    FVector3f& outPositionA = edgePositions.VertexPosition(edgeVertexIndexA);
    FVector3f& outPositionB = edgePositions.VertexPosition(edgeVertexIndexB);

    // Get the edge endpoints (as indices into the original mesh).
    const uint32 a = visibleEdges.edgeIndices[edgeVertexIndexA];
    const uint32 b = visibleEdges.edgeIndices[edgeVertexIndexB];

    if (a < 0 || b < 0 || a >= vertexCount || b >= vertexCount) {
      // If the indices are invalid, fill with default values.
      outPositionA = FVector3f::Zero();
      outPositionB = FVector3f::Zero();
      edgeVertexBuffer.SetVertexTangents(
          edgeVertexIndexA,
          FVector3f(0, 0, 1),
          FVector3f(0, 0, 1),
          FVector3f(0, 0, 1));
      edgeVertexBuffer.SetVertexTangents(
          edgeVertexIndexB,
          FVector3f(0, 0, 1),
          FVector3f(0, 0, 1),
          FVector3f(0, 0, 1));
      continue;
    }

    outPositionA = positionView[a];
    outPositionB = positionView[b];

    const uint8_t edgeType = visibleEdges.edgeTypes[i];
    // does this really have to be encoded this way?
    float t = float(edgeType) / 255.0f;

    edgeVertexBuffer.SetVertexUV(edgeVertexIndexA, 0, FVector2f(t, 0.0f));
    edgeVertexBuffer.SetVertexUV(edgeVertexIndexB, 0, FVector2f(t, 0.0f));
  }

  const CesiumGltf::Accessor* pSilhouetteNormalAccessor =
      Model::getSafe(&model.accessors, extension.silhouetteNormals);
  if (!pSilhouetteNormalAccessor) {
    return std::move(pRenderData);
  }

  switch (pSilhouetteNormalAccessor->componentType) {
  case CesiumGltf::Accessor::ComponentType::BYTE: {
    break;
  }
  case CesiumGltf::Accessor::ComponentType::SHORT: {
    break;
  }
  case CesiumGltf::Accessor::ComponentType::FLOAT: {
    break;
  }
  default:
    // TODO: log
    break;
  }

  // Needed for SILHOUETTE edges, if present.
  // TODO: supported types are float, byte, short.
  CesiumGltf::AccessorView<glm::vec3> silhouetteNormalAccessor(
      model,
      edgeExtension.silhouetteNormals);
  bool hasSilhouetteNormals = silhouetteNormalAccessor.status() ==
                              CesiumGltf::AccessorViewStatus::Valid;
}
} // namespace
