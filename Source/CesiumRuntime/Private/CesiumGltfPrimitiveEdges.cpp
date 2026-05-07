// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGltfPrimitiveEdges.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfLinesComponent.h"
#include "CesiumRuntime.h"
#include "CesiumTextureResource.h"
#include "CesiumTextureUtility.h"
#include "ExtensionImageAssetUnreal.h"
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

/*static*/ UCesiumGltfLinesComponent*
CesiumGltfPrimitiveEdges::createInMainThread(
    UCesiumGltfComponent* pComponent,
    const FName& componentName,
    UMaterialInstanceDynamic* pEdgeMaterial,
    TUniquePtr<FStaticMeshRenderData>&& pRenderData) {
  UCesiumGltfLinesComponent* pLineComponent =
      NewObject<UCesiumGltfLinesComponent>(
          pComponent,
          FName(componentName.ToString() + TEXT("Edges")));

  pLineComponent->bUseDefaultCollision = false;
  pLineComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
  // pLineComponent->SetFlags(
  //     RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);

  UStaticMesh* pStaticMesh =
      NewObject<UStaticMesh>(pLineComponent, componentName);
  pStaticMesh->bSupportRayTracing = false;
  pLineComponent->SetStaticMesh(pStaticMesh);

  pStaticMesh->SetFlags(
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
  pStaticMesh->NeverStream = true;

  pStaticMesh->SetRenderData(std::move(pRenderData));
  pStaticMesh->AddMaterial(pEdgeMaterial);
  pStaticMesh->SetLightingGuid();

  return pLineComponent;
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

  int64_t edgeIndex = 0;
  int32_t silhouetteEdgeCount = 0;

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
            "Invalid accessor for silhouette normals in EXT_mesh_primitive_edges_visibility; skipping."));

    // Ignore silhouette edges as a fallback.
    for (size_t i = 0; i < silhouetteEdgeCount; i++) {
      edgeVertexBuffer.SetVertexUV(i, 0, FVector2f::Zero());
    }
    return;
  }

  for (size_t i = 0; i < silhouetteEdgeCount; i++) {
    CESIUM_ASSERT(
        visibleEdges.edgeTypes[i] ==
        ExtensionExtMeshPrimitiveEdgeVisibility::Visibility::SILHOUETTE);

    if (int64_t(i) * 2 + 1 >= silhouetteNormals.size()) {
      // Protect against out-of-bounds access.
      break;
    }

    glm::vec3 normalA, normalB;
    if constexpr (std::is_same_v<TNormal, glm::i8vec3>) {
      normalA = glm::vec3(silhouetteNormals[i * 2]) /
                float(std::numeric_limits<int8_t>::max());
      normalB = glm::vec3(silhouetteNormals[i * 2 + 1]) /
                float(std::numeric_limits<int8_t>::max());
    } else if constexpr (std::is_same_v<TNormal, glm::i16vec3>) {
      normalA = glm::vec3(silhouetteNormals[i * 2]) /
                float(std::numeric_limits<int16_t>::max());
      normalB = glm::vec3(silhouetteNormals[i * 2 + 1]) /
                float(std::numeric_limits<int16_t>::max());
    } else {
      normalA = silhouetteNormals[i * 2];
      normalB = silhouetteNormals[i * 2 + 1];
    }

    // Silhouette edge normals are stored via vertex tangents.
    edgeVertexBuffer.SetVertexTangents(
        i,
        FVector3f(normalA.x, normalA.y, normalA.z),
        FVector3f(normalB.x, normalB.y, normalB.z),
        FVector3f::Zero());
  }
}

template <typename TIndex>
TUniquePtr<FStaticMeshRenderData> createInWorkerThreadImpl(
    const CesiumGltf::Model& model,
    const CesiumGltf::AccessorView<FVector3f>& positionView,
    const CesiumGltf::AccessorView<TIndex>& indicesView,
    const CesiumGltf::ExtensionExtMeshPrimitiveEdgeVisibility& extension) {
  CesiumGltf::AccessorView<uint8_t> visibilityView(model, extension.visibility);
  if (visibilityView.status() != CesiumGltf::AccessorViewStatus::Valid) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "Invalid visibility accessor in EXT_mesh_primitive_edges_visibility; skipping."));
    return nullptr;
  }

  VisibleEdgeResult visibleEdges =
      extractVisibleEdges(visibilityView, indicesView);
  if (visibleEdges.edgeTypes.empty()) {
    // No edges were detected.
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
  FStaticMeshVertexBuffer& edgeVertexBuffer =
      lodResources.VertexBuffers.StaticMeshVertexBuffer;
  edgeVertexBuffer.Init(totalEdgeVertices, 1, false);

  TArray<uint32> indices;
  indices.Reserve(totalEdgeVertices);

  glm::vec3 minPosition{std::numeric_limits<float>::max()};
  glm::vec3 maxPosition{std::numeric_limits<float>::lowest()};

  pRenderData->Bounds.SphereRadius = 0.0f;

  TIndex vertexCount = TIndex(positionView.size());
  for (size_t i = 0; i < edgeCount; i++) {
    uint32 edgeVertexIndexA = i * 2;
    uint32 edgeVertexIndexB = i * 2 + 1;

    indices.Add(edgeVertexIndexA);
    indices.Add(edgeVertexIndexB);

    FVector3f& outPositionA = edgePositions.VertexPosition(edgeVertexIndexA);
    FVector3f& outPositionB = edgePositions.VertexPosition(edgeVertexIndexB);

    // Get the endpoints of the edge (as indices into the original mesh).
    const uint32 a = visibleEdges.edgeVertexIndices[edgeVertexIndexA];
    const uint32 b = visibleEdges.edgeVertexIndices[edgeVertexIndexB];

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

    // TODO account for position stuff...
    outPositionA.X = positionA.X * CesiumPrimitiveData::positionScaleFactor;
    outPositionA.Y = -positionA.Y * CesiumPrimitiveData::positionScaleFactor;
    outPositionA.Z = positionA.Z * CesiumPrimitiveData::positionScaleFactor;

    outPositionB.X = positionB.X * CesiumPrimitiveData::positionScaleFactor;
    outPositionB.Y = -positionB.Y * CesiumPrimitiveData::positionScaleFactor;
    outPositionB.Z = positionB.Z * CesiumPrimitiveData::positionScaleFactor;

    minPosition.x = glm::min<float>(minPosition.x, outPositionA.X);
    minPosition.y = glm::min<float>(minPosition.y, outPositionA.Y);
    minPosition.z = glm::min<float>(minPosition.z, outPositionA.Z);
    maxPosition.x = glm::max<float>(maxPosition.x, outPositionA.X);
    maxPosition.y = glm::max<float>(maxPosition.y, outPositionA.Y);
    maxPosition.z = glm::max<float>(maxPosition.z, outPositionA.Z);

    minPosition.x = glm::min<float>(minPosition.x, outPositionB.X);
    minPosition.y = glm::min<float>(minPosition.y, outPositionB.Y);
    minPosition.z = glm::min<float>(minPosition.z, outPositionB.Z);

    maxPosition.x = glm::max<float>(maxPosition.x, outPositionB.X);
    maxPosition.y = glm::max<float>(maxPosition.y, outPositionB.Y);
    maxPosition.z = glm::max<float>(maxPosition.z, outPositionB.Z);

    const uint8_t edgeType = visibleEdges.edgeTypes[i];
    // does this really have to be encoded this way?
    float t = float(edgeType) / 255.0f;

    edgeVertexBuffer.SetVertexUV(edgeVertexIndexA, 0, FVector2f(t, 0.0f));
    edgeVertexBuffer.SetVertexUV(edgeVertexIndexB, 0, FVector2f(t, 0.0f));

    pRenderData->Bounds.SphereRadius = FMath::Max(
        (FVector(outPositionA) - pRenderData->Bounds.Origin).Size(),
        pRenderData->Bounds.SphereRadius);
    pRenderData->Bounds.SphereRadius = FMath::Max(
        (FVector(outPositionB) - pRenderData->Bounds.Origin).Size(),
        pRenderData->Bounds.SphereRadius);
  }

  FBox aaBox(
      FVector3d(minPosition.x, minPosition.y, minPosition.z),
      FVector3d(maxPosition.x, maxPosition.y, maxPosition.z));

  aaBox.GetCenterAndExtents(
      pRenderData->Bounds.Origin,
      pRenderData->Bounds.BoxExtent);

  FStaticMeshSectionArray& Sections = lodResources.Sections;
  FStaticMeshSection& section = Sections.AddDefaulted_GetRef();
  // This will be ignored since the primitive contains lines.
  section.NumTriangles = 1;
  section.FirstIndex = 0;
  section.MinVertexIndex = 0;
  section.MaxVertexIndex = totalEdgeVertices - 1;
  section.bEnableCollision = false;
  section.bCastShadow = true;
  section.MaterialIndex = 0;

  lodResources.IndexBuffer.SetIndices(
      indices,
      totalEdgeVertices >= std::numeric_limits<uint16>::max()
          ? EIndexBufferStride::Type::Force32Bit
          : EIndexBufferStride::Type::Force16Bit);
  lodResources.bHasDepthOnlyIndices = false;
  lodResources.bHasReversedIndices = false;
  lodResources.bHasReversedDepthOnlyIndices = false;

  const CesiumGltf::Accessor* pSilhouetteNormalAccessor =
      Model::getSafe(&model.accessors, extension.silhouetteNormals);
  if (!pSilhouetteNormalAccessor) {
    return std::move(pRenderData);
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
  case CesiumGltf::Accessor::ComponentType::FLOAT: {
    CesiumGltf::AccessorView<glm::vec3> silhouetteNormalView(
        model,
        *pSilhouetteNormalAccessor);
    populateSilhouetteNormals(
        visibleEdges,
        silhouetteNormalView,
        edgeVertexBuffer);
    break;
  }
  default:
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "Unsupported accessor component type for silhouette normals in EXT_mesh_primitive_edges_visibility; skipping."));
    return nullptr;
    break;
  }

  return std::move(pRenderData);
}
} // namespace
