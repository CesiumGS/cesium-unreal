// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGltfPrimitiveEdges.h"
#include "CesiumPrimitive.h"
#include "CesiumRuntime.h"
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/ExtensionExtMeshPrimitiveEdgeVisibility.h>
#include <CesiumGltf/Model.h>
#include <cstdlib>
#include <limits>
#include <type_traits>

using namespace CesiumGltf;

namespace {
/**
 * Edge data extracted from EXT_mesh_primitive_edge_visibility.
 */
struct VisibleEdgeResult {
  /**
   * The extracted edge type from the visibility accessor.
   */
  TArray<uint8_t> edgeTypes;
  /**
   * The indices of the mesh vertices that make up the edges (2 per edge).
   */
  TArray<uint32> edgeVertexIndices;
  /**
   * The indices of any silhouette edges that appeared in edgeTypes.
   */
  TArray<int32> silhouetteEdgeIndices;
};

template <typename TIndex>
VisibleEdgeResult extractVisibleEdges(
    const CesiumGltf::AccessorView<uint8_t>& visibility,
    const CesiumGltf::AccessorView<TIndex>& indices);

template <typename TIndex>
TArray<uint32> extractLineStringIndices(
    const CesiumGltf::AccessorView<FVector3f>& positionView,
    const CesiumGltf::AccessorView<TIndex>& indicesView);

void populateVertexBufferForVisibleEdges(
    const CesiumGltf::Model& model,
    const VisibleEdgeResult& visibleEdges,
    const CesiumGltf::AccessorView<FVector3f>& positionView,
    const FColor& color,
    int32_t silhouetteNormalAccessorIndex,
    FPositionVertexBuffer& positionBuffer,
    FColorVertexBuffer& colorBuffer,
    FStaticMeshVertexBuffer& vertexBuffer,
    TArray<uint32>& indices);

void populateVertexBufferForLineStrings(
    const CesiumGltf::Model& model,
    const TArray<uint32>& allLineStringIndices,
    const TArray<int32>& lineStringOffsets,
    const TArray<int32>& materialIndices,
    const CesiumGltf::AccessorView<FVector3f>& positionView,
    const FColor& fallbackColor,
    FPositionVertexBuffer& positionBuffer,
    FColorVertexBuffer& colorBuffer,
    FStaticMeshVertexBuffer& vertexBuffer,
    TArray<uint32>& indices);

FColor getBaseColorFromMaterial(const Material& material);

} // namespace

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

  // First, extract edges from the visibility stream if present.
  VisibleEdgeResult visibleEdges;

  if (extension.visibility >= 0) {
    CesiumGltf::AccessorView<uint8_t> visibilityView(
        model,
        extension.visibility);

    // Remake the index accessor view here to avoid templates in the .h file.
    switch (pIndexAccessor->componentType) {
    case CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE: {
      CesiumGltf::AccessorView<uint8_t> indicesView(model, *pIndexAccessor);
      visibleEdges = extractVisibleEdges(visibilityView, indicesView);
      break;
    }
    case CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT: {
      CesiumGltf::AccessorView<uint16_t> indicesView(model, *pIndexAccessor);
      visibleEdges = extractVisibleEdges(visibilityView, indicesView);
      break;
    }
    case CesiumGltf::Accessor::ComponentType::UNSIGNED_INT: {
      CesiumGltf::AccessorView<uint32_t> indicesView(model, *pIndexAccessor);
      visibleEdges = extractVisibleEdges(visibilityView, indicesView);
      break;
    }
    default:
      break;
    }
  }

  const Material* pExtensionMaterial =
      Model::getSafe(&model.materials, extension.material);
  int32_t resolvedMaterialIndex =
      pExtensionMaterial ? extension.material : primitive.material;

  // Then, handle line strings.
  TArray<uint32> allLineStringIndices;

  // Line strings may specify their own material overrides, so track which
  // material each string uses. lineStringOffsets behaves like arrayOffsets in
  // EXT_structural_metadata, where the number of points in line string `i` can
  // be derived by `offsets[i + 1] - offsets[i]`.
  TArray<int32> lineStringOffsets;
  TArray<int32> materialIndices;
  int32 currentOffset = 0;

  for (const CesiumGltf::LineString& lineString : extension.lineStrings) {
    pIndexAccessor = Model::getSafe(&model.accessors, lineString.indices);
    if (!pIndexAccessor) {
      UE_LOG(
          LogCesium,
          Error,
          TEXT(
              "Invalid index accessor in line string from EXT_mesh_primitive_edges_visibility; "
              "skipping."));
      continue;
    }

    TArray<uint32> lineStringIndices;
    switch (pIndexAccessor->componentType) {
    case CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE: {
      CesiumGltf::AccessorView<uint8_t> indicesView(model, *pIndexAccessor);
      lineStringIndices = extractLineStringIndices(positionView, indicesView);
      break;
    }
    case CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT: {
      CesiumGltf::AccessorView<uint16_t> indicesView(model, *pIndexAccessor);
      lineStringIndices = extractLineStringIndices(positionView, indicesView);
      break;
    }
    case CesiumGltf::Accessor::ComponentType::UNSIGNED_INT: {
      CesiumGltf::AccessorView<uint32_t> indicesView(model, *pIndexAccessor);
      lineStringIndices = extractLineStringIndices(positionView, indicesView);
      break;
    }
    default:
      break;
    }

    if (lineStringIndices.Num()) {
      lineStringOffsets.Add(currentOffset);
      currentOffset += lineStringIndices.Num();
      allLineStringIndices.Append(std::move(lineStringIndices));

      const Material* pMaterial =
          Model::getSafe(&model.materials, lineString.material);
      materialIndices.Add(
          pMaterial ? lineString.material : resolvedMaterialIndex);
    }
  }
  lineStringOffsets.Add(currentOffset);

  TUniquePtr<FStaticMeshRenderData> pRenderData =
      MakeUnique<FStaticMeshRenderData>();
  pRenderData->AllocateLODResources(1);

  FStaticMeshLODResources& lodResources = pRenderData->LODResources[0];

  int32 totalEdgeVertices =
      visibleEdges.edgeVertexIndices.Num() + allLineStringIndices.Num();

  FPositionVertexBuffer& edgePositions =
      lodResources.VertexBuffers.PositionVertexBuffer;
  edgePositions.Init(totalEdgeVertices, false);

  // Edge type is passed through as a UV coordinate in the edge vertex buffer.
  // If silhouette edges are used, then two UV sets are used; the other three
  // floats are used to pass the second silhouette normal for that edge. (See
  // populateSilhouetteNormals)
  int32 numTexCoords = visibleEdges.silhouetteEdgeIndices.IsEmpty() ? 1 : 2;

  FStaticMeshVertexBuffer& edgeVertexBuffer =
      lodResources.VertexBuffers.StaticMeshVertexBuffer;
  edgeVertexBuffer.Init(totalEdgeVertices, numTexCoords, false);

  FColorVertexBuffer& edgeColors = lodResources.VertexBuffers.ColorVertexBuffer;
  edgeColors.Init(totalEdgeVertices, false);

  FColor resolvedEdgeColor = getBaseColorFromMaterial(
      Model::getSafe(model.materials, resolvedMaterialIndex));
  lodResources.bHasColorVertexData = true;

  TArray<uint32> indices;
  indices.Reserve(totalEdgeVertices);

  // First, iterate over the edges from the visibility stream.
  populateVertexBufferForVisibleEdges(
      model,
      visibleEdges,
      positionView,
      resolvedEdgeColor,
      extension.silhouetteNormals,
      edgePositions,
      edgeColors,
      edgeVertexBuffer,
      indices);

  // Then, iterate over the edges from the explicit line strings.
  populateVertexBufferForLineStrings(
      model,
      allLineStringIndices,
      lineStringOffsets,
      materialIndices,
      positionView,
      resolvedEdgeColor,
      edgePositions,
      edgeColors,
      edgeVertexBuffer,
      indices);

  lodResources.IndexBuffer.SetIndices(
      indices,
      totalEdgeVertices >= std::numeric_limits<uint16>::max()
          ? EIndexBufferStride::Type::Force32Bit
          : EIndexBufferStride::Type::Force16Bit);
  lodResources.bHasDepthOnlyIndices = false;
  lodResources.bHasReversedIndices = false;
  lodResources.bHasReversedDepthOnlyIndices = false;

  FStaticMeshSectionArray& sections = lodResources.Sections;
  FStaticMeshSection& section = sections.AddDefaulted_GetRef();
  // This will be ignored since the primitive contains lines.
  section.NumTriangles = indices.Num() / 3;
  section.FirstIndex = 0;
  section.MinVertexIndex = 0;
  section.MaxVertexIndex = totalEdgeVertices - 1;
  section.bEnableCollision = false;
  section.bCastShadow = true;
  section.MaterialIndex = 0;

  return pRenderData;
}

namespace {

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
        Error,
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
      auto divResult = std::lldiv(edgeIndex, 4);
      edgeIndex++;

      uint32 byteIndex = static_cast<uint32>(divResult.quot);
      uint32 bitPairOffset = static_cast<uint32>(divResult.rem * 2);
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
        result.edgeTypes.Add(edgeType);
        result.edgeVertexIndices.Add(uint32(a));
        result.edgeVertexIndices.Add(uint32(b));
        break;
      default:
        // Skip hidden and repeated edges.
        continue;
      }

      if (edgeType ==
          ExtensionExtMeshPrimitiveEdgeVisibility::Visibility::SILHOUETTE) {
        result.silhouetteEdgeIndices.Add(result.edgeTypes.Num() - 1);
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
  int32 silhouetteEdgeCount = visibleEdges.silhouetteEdgeIndices.Num();

  if (silhouetteNormals.status() != AccessorViewStatus::Valid) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Invalid accessor for silhouette normals in EXT_mesh_primitive_edges_visibility; "
            "silhouette edges will be hidden."));

    constexpr float hidden(
        ExtensionExtMeshPrimitiveEdgeVisibility::Visibility::HIDDEN);

    for (int32 i = 0; i < silhouetteEdgeCount; i++) {
      int32 edgeIndex = visibleEdges.silhouetteEdgeIndices[i];
      uint32 edgeVertexIndexA = uint32(edgeIndex * 2);
      uint32 edgeVertexIndexB = uint32(edgeIndex * 2 + 1);
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
    int32 edgeIndex = visibleEdges.silhouetteEdgeIndices[i];
    uint32 edgeVertexIndexA = uint32(edgeIndex * 2);
    uint32 edgeVertexIndexB = uint32(edgeIndex * 2 + 1);
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
TArray<uint32> extractLineStringIndices(
    const CesiumGltf::AccessorView<FVector3f>& positionView,
    const CesiumGltf::AccessorView<TIndex>& indicesView) {
  TArray<uint32> indices;
  if (indicesView.status() != AccessorViewStatus::Valid) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Invalid index accessor for line string in EXT_mesh_primitive_edges_visibility; "
            "skipping."));
    return indices;
  }

  indices.Reserve(indicesView.size());

  TIndex vertexCount = TIndex(positionView.size());
  int64_t maximumEdgeCount = indicesView.size() - 1;

  // If we encounter the primitive restart value (i.e., the maximum possible
  // value for the specified component type), skip to the next edge.
  constexpr uint32_t primitiveRestartValue(std::numeric_limits<TIndex>::max());
  for (int64_t i = 0; i < maximumEdgeCount; i++) {
    // Get the endpoints of the edge (as indices into the original mesh).
    uint32 a = indicesView[i];
    uint32 b = indicesView[i + 1];

    if (a == primitiveRestartValue || a >= vertexCount ||
        b == primitiveRestartValue || b >= vertexCount) {
      continue;
    }

    indices.Add(a);
    indices.Add(b);
  }

  return indices;
}

void fillWithDefaultValues(
    FVector3f& outPosition,
    FStaticMeshVertexBuffer& vertexBuffer,
    uint32 index) {
  outPosition = FVector3f::Zero();
  vertexBuffer.SetVertexUV(index, 0, FVector2f::Zero());
}

void populateVertexBufferForVisibleEdges(
    const CesiumGltf::Model& model,
    const VisibleEdgeResult& visibleEdges,
    const CesiumGltf::AccessorView<FVector3f>& positionView,
    const FColor& color,
    int32_t silhouetteNormalAccessorIndex,
    FPositionVertexBuffer& positionBuffer,
    FColorVertexBuffer& colorBuffer,
    FStaticMeshVertexBuffer& vertexBuffer,
    TArray<uint32>& indices) {
  if (visibleEdges.edgeTypes.IsEmpty()) {
    return;
  }

  uint32 vertexCount = static_cast<uint32>(positionView.size());
  for (size_t i = 0; i < visibleEdges.edgeTypes.Num(); i++) {
    uint32 edgeVertexIndexA = i * 2;
    uint32 edgeVertexIndexB = i * 2 + 1;

    indices.Add(edgeVertexIndexA);
    indices.Add(edgeVertexIndexB);

    FVector3f& outPositionA = positionBuffer.VertexPosition(edgeVertexIndexA);
    FVector3f& outPositionB = positionBuffer.VertexPosition(edgeVertexIndexB);

    // Get the endpoints of the edge (as indices into the original mesh).
    uint32 a = visibleEdges.edgeVertexIndices[edgeVertexIndexA];
    uint32 b = visibleEdges.edgeVertexIndices[edgeVertexIndexB];

    if (a >= vertexCount || b >= vertexCount) {
      // If the indices are invalid, fill with default values.
      fillWithDefaultValues(outPositionA, vertexBuffer, edgeVertexIndexA);
      fillWithDefaultValues(outPositionB, vertexBuffer, edgeVertexIndexB);
      continue;
    }

    FVector3f positionA = scalePositionForUnreal(positionView[a]);
    FVector3f positionB = scalePositionForUnreal(positionView[b]);
    outPositionA = positionA;
    outPositionB = positionB;

    FColor& colorA = colorBuffer.VertexColor(edgeVertexIndexA);
    FColor& colorB = colorBuffer.VertexColor(edgeVertexIndexB);
    colorA = color;
    colorB = color;

    float edgeType = float(visibleEdges.edgeTypes[i]);
    vertexBuffer.SetVertexUV(edgeVertexIndexA, 0, FVector2f(edgeType, 0.0f));
    vertexBuffer.SetVertexUV(edgeVertexIndexB, 0, FVector2f(edgeType, 0.0f));
  }

  if (visibleEdges.silhouetteEdgeIndices.IsEmpty()) {
    return;
  }

  const CesiumGltf::Accessor* pSilhouetteNormalAccessor =
      Model::getSafe(&model.accessors, silhouetteNormalAccessorIndex);
  int32_t componentType = pSilhouetteNormalAccessor
                              ? pSilhouetteNormalAccessor->componentType
                              : CesiumGltf::Accessor::ComponentType::FLOAT;
  // populateSilhouetteNormals will handle when the accessor is invalid.
  switch (componentType) {
  case CesiumGltf::Accessor::ComponentType::BYTE: {
    CesiumGltf::AccessorView<glm::i8vec3> silhouetteNormalView(
        model,
        silhouetteNormalAccessorIndex);
    populateSilhouetteNormals(visibleEdges, silhouetteNormalView, vertexBuffer);
    break;
  }
  case CesiumGltf::Accessor::ComponentType::SHORT: {
    CesiumGltf::AccessorView<glm::i16vec3> silhouetteNormalView(
        model,
        silhouetteNormalAccessorIndex);
    populateSilhouetteNormals(visibleEdges, silhouetteNormalView, vertexBuffer);
    break;
  }
  case CesiumGltf::Accessor::ComponentType::FLOAT:
  default: {
    CesiumGltf::AccessorView<glm::vec3> silhouetteNormalView(
        model,
        silhouetteNormalAccessorIndex);
    populateSilhouetteNormals(visibleEdges, silhouetteNormalView, vertexBuffer);
    break;
  }
  }
}

void populateVertexBufferForLineStrings(
    const CesiumGltf::Model& model,
    const TArray<uint32>& allLineStringIndices,
    const TArray<int32>& lineStringOffsets,
    const TArray<int32>& materialIndices,
    const CesiumGltf::AccessorView<FVector3f>& positionView,
    const FColor& fallbackColor,
    FPositionVertexBuffer& positionBuffer,
    FColorVertexBuffer& colorBuffer,
    FStaticMeshVertexBuffer& vertexBuffer,
    TArray<uint32>& indices) {
  if (allLineStringIndices.IsEmpty()) {
    return;
  }

  uint32 vertexCount = static_cast<uint32>(positionView.size());

  // Offset for indices that are already present from
  // populateVertexBufferForVisibleEdges.
  uint32 totalVertexOffset = uint32(indices.Num());
  int32 lineStringCount = lineStringOffsets.Num() - 1;
  CESIUM_ASSERT(materialIndices.Num() == lineStringCount);

  for (int32 l = 0; l < lineStringCount; l++) {
    // Line strings may specify their own material to override the one used by
    // the extension.
    const Material* pLineStringMaterial =
        model.getSafe(&model.materials, materialIndices[l]);
    FColor lineStringColor =
        pLineStringMaterial ? getBaseColorFromMaterial(*pLineStringMaterial)
                            : fallbackColor;

    int32 lineStringBegin = lineStringOffsets[l];
    int32 lineStringEnd = lineStringOffsets[l + 1];
    int32 verticesInLineString = lineStringEnd - lineStringBegin;
    for (int32 i = 0; i < verticesInLineString; i += 2) {
      uint32 relativeIndexA = i;
      uint32 relativeIndexB = i + 1;

      uint32 writeIndexA = totalVertexOffset + relativeIndexA;
      uint32 writeIndexB = totalVertexOffset + relativeIndexB;
      indices.Add(writeIndexA);
      indices.Add(writeIndexB);

      FVector3f& outPositionA = positionBuffer.VertexPosition(writeIndexA);
      FVector3f& outPositionB = positionBuffer.VertexPosition(writeIndexB);

      // Get the endpoints of the edge (as indices into the original mesh).
      uint32 a = allLineStringIndices[lineStringBegin + relativeIndexA];
      uint32 b = allLineStringIndices[lineStringBegin + relativeIndexB];

      if (a >= vertexCount || b >= vertexCount) {
        // If the indices are invalid, fill with default values.
        fillWithDefaultValues(outPositionA, vertexBuffer, writeIndexA);
        fillWithDefaultValues(outPositionB, vertexBuffer, writeIndexB);
        continue;
      }

      FVector3f positionA = scalePositionForUnreal(positionView[a]);
      FVector3f positionB = scalePositionForUnreal(positionView[b]);
      outPositionA = positionA;
      outPositionB = positionB;

      FColor& colorA = colorBuffer.VertexColor(writeIndexA);
      FColor& colorB = colorBuffer.VertexColor(writeIndexB);
      colorA = lineStringColor;
      colorB = lineStringColor;

      constexpr float edgeType =
          float(ExtensionExtMeshPrimitiveEdgeVisibility::Visibility::HARD_EDGE);
      vertexBuffer.SetVertexUV(writeIndexA, 0, FVector2f(edgeType, 0.0f));
      vertexBuffer.SetVertexUV(writeIndexB, 0, FVector2f(edgeType, 0.0f));
    }

    totalVertexOffset += verticesInLineString;
  }
}

FColor getBaseColorFromMaterial(const Material& material) {
  FLinearColor result(1.f, 1.f, 1.f, 1.f);

  const MaterialPBRMetallicRoughness& pbr =
      material.pbrMetallicRoughness.value_or(MaterialPBRMetallicRoughness());
  if (pbr.baseColorFactor.size() >= 3) {
    result.R = pbr.baseColorFactor[0];
    result.G = pbr.baseColorFactor[1];
    result.B = pbr.baseColorFactor[2];
    result.A = pbr.baseColorFactor.size() > 3 ? pbr.baseColorFactor[3] : 1.f;
  }

  return result.ToFColor(false);
}
} // namespace
