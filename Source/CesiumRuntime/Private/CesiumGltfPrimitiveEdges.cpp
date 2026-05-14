// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGltfPrimitiveEdges.h"
#include "CesiumRuntime.h"
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/ExtensionExtMeshPrimitiveEdgeVisibility.h>
#include <CesiumGltf/Model.h>
#include <limits>
#include <type_traits>

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
   * The extracted edge type from the visibility accessor.
   */
  std::vector<uint8_t> edgeTypes;
  /**
   * The indices of the mesh vertices that make up the edges (2 per edge).
   */
  std::vector<uint32_t> edgeVertexIndices;
  /**
   * The indices of any silhouette edges that appeared in edgeTypes.
   */
  std::vector<size_t> silhouetteEdgeIndices;
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
    const CesiumGltf::AccessorView<TIndex>& indices) {
  VisibleEdgeResult result;
  if (visibility.status() != CesiumGltf::AccessorViewStatus::Valid) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "Invalid visibility accessor in EXT_mesh_primitive_edges_visibility; skipping."));
    return result;
  }

  int64_t edgeIndex = 0;

  for (int64_t indexOffset = 0; indexOffset + 2 < indices.size();
       indexOffset += 3) {
    // Iterate over triangles (every three indices).
    for (uint8_t vertex = 0; vertex < 3; vertex++) {
      // Identify the next edge in the triangle.
      // For each triangle (v0, v1, v2), the bitfield encodes three visibility
      // values for the edges (v0:v1, v1:v2, v2:v0) in that order.
      uint8_t nextVertex = (vertex + 1) % 3;
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

      TIndex a = indices[indexOffset + vertex];
      TIndex b = indices[indexOffset + nextVertex];

      switch (edgeType) {
      case ExtensionExtMeshPrimitiveEdgeVisibility::Visibility::HARD_EDGE:
      case ExtensionExtMeshPrimitiveEdgeVisibility::Visibility::SILHOUETTE:
        result.edgeTypes.emplace_back(edgeType);
        result.edgeVertexIndices.emplace_back(a);
        result.edgeVertexIndices.emplace_back(b);
        break;
      default:
        // Skip hidden and repeated edges.
        continue;
      }

      if (edgeType ==
          ExtensionExtMeshPrimitiveEdgeVisibility::Visibility::SILHOUETTE) {
        result.silhouetteEdgeIndices.push_back(result.edgeTypes.size() - 1);
      }
    }
  }
  return result;
}

template <typename TNormal>
void populateSilhouetteNormals(
    const VisibleEdgeResult& visibleEdges,
    const CesiumGltf::AccessorView<TNormal>& silhouetteNormals,
    FStaticMeshVertexBuffer& edgeVertexBuffer) {
  size_t silhouetteEdgeCount = visibleEdges.silhouetteEdgeIndices.size();

  if (silhouetteNormals.status() != AccessorViewStatus::Valid) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "Invalid accessor for silhouette normals in EXT_mesh_primitive_edges_visibility;"
            "silhouette edges will be hidden."));

    constexpr float hidden(
        ExtensionExtMeshPrimitiveEdgeVisibility::Visibility::HIDDEN);

    for (size_t i = 0; i < silhouetteEdgeCount; i++) {
      size_t edgeIndex = visibleEdges.silhouetteEdgeIndices[i];
      uint32 edgeVertexIndexA = edgeIndex * 2;
      uint32 edgeVertexIndexB = edgeIndex * 2 + 1;
      edgeVertexBuffer.SetVertexUV(edgeVertexIndexA, hidden, FVector2f::Zero());
      edgeVertexBuffer.SetVertexUV(edgeVertexIndexB, hidden, FVector2f::Zero());
    }
    return;
  }

  auto writeSilhouetteNormals = [&edgeVertexBuffer](
                                    uint32 index,
                                    const glm::vec3& normalA,
                                    const glm::vec3& normalB) {
    constexpr float silhouette(
        ExtensionExtMeshPrimitiveEdgeVisibility::Visibility::SILHOUETTE);

    // Although it is tempting to pass both silhouette normals through the
    // tangents, the TangentX value gets transformed (or recomputed?) by the
    // time it is passed to the Unreal material. It is more reliable to pass the
    // value through the UV coordinates.
    edgeVertexBuffer.SetVertexTangents(
        index,
        FVector3f::Zero(),
        FVector3f::Zero(),
        FVector3f(normalA.x, -normalA.y, normalA.z));

    edgeVertexBuffer.SetVertexUV(index, 0, FVector2f(silhouette, normalB.x));
    edgeVertexBuffer.SetVertexUV(index, 1, FVector2f(-normalB.y, normalB.z));
  };

  for (size_t i = 0; i < silhouetteEdgeCount; i++) {
    size_t edgeIndex = visibleEdges.silhouetteEdgeIndices[i];
    uint32 edgeVertexIndexA = edgeIndex * 2;
    uint32 edgeVertexIndexB = edgeIndex * 2 + 1;
    CESIUM_ASSERT(
        visibleEdges.edgeTypes[edgeIndex] ==
        ExtensionExtMeshPrimitiveEdgeVisibility::Visibility::SILHOUETTE);

    if (int64_t(i) * 2 + 1 >= silhouetteNormals.size()) {
      // Protect against out-of-bounds access, ignoring the edge as a fallback.
      edgeVertexBuffer.SetVertexUV(edgeVertexIndexA, 0, FVector2f::Zero());
      edgeVertexBuffer.SetVertexUV(edgeVertexIndexB, 0, FVector2f::Zero());
      continue;
    }

    // Each silhouette edge corresponds to a pair of two normals.
    glm::vec3 normalA = glm::vec3(silhouetteNormals[i * 2]),
              normalB = glm::vec3(silhouetteNormals[i * 2 + 1]);

    // normalize() handles BYTE and SHORT type normals, too.
    normalA = glm::normalize(normalA);
    normalB = glm::normalize(normalB);

    writeSilhouetteNormals(edgeVertexIndexA, normalA, normalB);
    writeSilhouetteNormals(edgeVertexIndexB, normalA, normalB);
  }
}

template <typename TIndex>
TUniquePtr<FStaticMeshRenderData> createInWorkerThreadImpl(
    const CesiumGltf::Model& model,
    const CesiumGltf::AccessorView<FVector3f>& positionView,
    const CesiumGltf::AccessorView<TIndex>& indicesView,
    const CesiumGltf::ExtensionExtMeshPrimitiveEdgeVisibility& extension) {
  CesiumGltf::AccessorView<uint8_t> visibilityView(model, extension.visibility);

  VisibleEdgeResult visibleEdges =
      extractVisibleEdges(visibilityView, indicesView);

  if (visibleEdges.edgeTypes.empty()) {
    return nullptr;
  }

  TUniquePtr<FStaticMeshRenderData> pRenderData =
      MakeUnique<FStaticMeshRenderData>();
  pRenderData->AllocateLODResources(1);

  FStaticMeshLODResources& lodResources = pRenderData->LODResources[0];

  size_t edgeCount = visibleEdges.edgeTypes.size();
  size_t totalEdgeVertices = visibleEdges.edgeVertexIndices.size();

  FPositionVertexBuffer& edgePositions =
      lodResources.VertexBuffers.PositionVertexBuffer;
  edgePositions.Init(totalEdgeVertices, false);

  // Edge type is passed through as a UV coordinate in the edge vertex buffer.
  // If silhouette edges are used, then two UV sets are used; the other three
  // floats are used to pass the second silhouette normal for that edge. (See
  // populateSilhouetteNormals)
  int32 numTexCoords = 1 + int32(!visibleEdges.silhouetteEdgeIndices.empty());

  FStaticMeshVertexBuffer& edgeVertexBuffer =
      lodResources.VertexBuffers.StaticMeshVertexBuffer;
  edgeVertexBuffer.Init(totalEdgeVertices, numTexCoords, false);

  TArray<uint32> indices;
  indices.Reserve(totalEdgeVertices);

  glm::vec3 minPosition{std::numeric_limits<float>::max()};
  glm::vec3 maxPosition{std::numeric_limits<float>::lowest()};

  TIndex vertexCount = TIndex(positionView.size());
  for (size_t i = 0; i < edgeCount; i++) {
    uint32 edgeVertexIndexA = i * 2;
    uint32 edgeVertexIndexB = i * 2 + 1;

    indices.Add(edgeVertexIndexA);
    indices.Add(edgeVertexIndexB);

    FVector3f& outPositionA = edgePositions.VertexPosition(edgeVertexIndexA);
    FVector3f& outPositionB = edgePositions.VertexPosition(edgeVertexIndexB);

    // Get the endpoints of the edge (as indices into the original mesh).
    uint32 a = visibleEdges.edgeVertexIndices[edgeVertexIndexA];
    uint32 b = visibleEdges.edgeVertexIndices[edgeVertexIndexB];

    if (a >= vertexCount || b >= vertexCount) {
      // If the indices are invalid, fill with default values.
      outPositionA = FVector3f::Zero();
      outPositionB = FVector3f::Zero();
      edgeVertexBuffer.SetVertexUV(edgeVertexIndexA, 0, FVector2f::Zero());
      edgeVertexBuffer.SetVertexUV(edgeVertexIndexB, 0, FVector2f::Zero());
      continue;
    }

    FVector3f positionA = positionView[a];
    FVector3f positionB = positionView[b];

    outPositionA.X = positionA.X * CesiumPrimitiveData::positionScaleFactor;
    outPositionA.Y = -positionA.Y * CesiumPrimitiveData::positionScaleFactor;
    outPositionA.Z = positionA.Z * CesiumPrimitiveData::positionScaleFactor;

    outPositionB.X = positionB.X * CesiumPrimitiveData::positionScaleFactor;
    outPositionB.Y = -positionB.Y * CesiumPrimitiveData::positionScaleFactor;
    outPositionB.Z = positionB.Z * CesiumPrimitiveData::positionScaleFactor;

    uint8_t edgeType = visibleEdges.edgeTypes[i];

    edgeVertexBuffer.SetVertexUV(
        edgeVertexIndexA,
        0,
        FVector2f(float(edgeType)));
    edgeVertexBuffer.SetVertexUV(
        edgeVertexIndexB,
        0,
        FVector2f(float(edgeType)));
  }

  lodResources.IndexBuffer.SetIndices(
      indices,
      totalEdgeVertices >= std::numeric_limits<uint16>::max()
          ? EIndexBufferStride::Type::Force32Bit
          : EIndexBufferStride::Type::Force16Bit);
  lodResources.bHasDepthOnlyIndices = false;
  lodResources.bHasReversedIndices = false;
  lodResources.bHasReversedDepthOnlyIndices = false;

  FStaticMeshSectionArray& Sections = lodResources.Sections;
  FStaticMeshSection& section = Sections.AddDefaulted_GetRef();
  // This will be ignored since the primitive contains lines.
  section.NumTriangles = vertexCount / 3;
  section.FirstIndex = 0;
  section.MinVertexIndex = 0;
  section.MaxVertexIndex = totalEdgeVertices - 1;
  section.bEnableCollision = false;
  section.bCastShadow = true;
  section.MaterialIndex = 0;

  const CesiumGltf::Accessor* pSilhouetteNormalAccessor =
      Model::getSafe(&model.accessors, extension.silhouetteNormals);
  if (!pSilhouetteNormalAccessor) {
    return pRenderData;
  }

  switch (pSilhouetteNormalAccessor->componentType) {
  case CesiumGltf::Accessor::ComponentType::BYTE: {
    CesiumGltf::AccessorView<glm::i8vec3> silhouetteNormalView(
        model,
        *pSilhouetteNormalAccessor);
    populateSilhouetteNormals(
        visibleEdges,
        silhouetteNormalView,
        edgeVertexBuffer);
    break;
  }
  case CesiumGltf::Accessor::ComponentType::SHORT: {
    CesiumGltf::AccessorView<glm::i16vec3> silhouetteNormalView(
        model,
        *pSilhouetteNormalAccessor);
    populateSilhouetteNormals(
        visibleEdges,
        silhouetteNormalView,
        edgeVertexBuffer);
    break;
  }
  case CesiumGltf::Accessor::ComponentType::FLOAT:
  default: {
    CesiumGltf::AccessorView<glm::vec3> silhouetteNormalView(
        model,
        *pSilhouetteNormalAccessor);
    populateSilhouetteNormals(
        visibleEdges,
        silhouetteNormalView,
        edgeVertexBuffer);
    break;
  }
  }

  return pRenderData;
}
} // namespace
