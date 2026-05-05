// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGltfLinesComponent.h"
#include "CesiumGltfLinesSceneProxy.h"
#include "SceneInterface.h"

#include <CesiumGltf/ExtensionExtMeshPrimitiveEdgeVisibility.h>

using namespace CesiumGltf;

// Sets default values for this component's properties
UCesiumGltfLinesComponent::UCesiumGltfLinesComponent() {}

UCesiumGltfLinesComponent::~UCesiumGltfLinesComponent() {}

FPrimitiveSceneProxy* UCesiumGltfLinesComponent::CreateSceneProxy() {
  if (!IsValid(this)) {
    return nullptr;
  }

  return new FCesiumGltfLinesSceneProxy(
      this,
      FSceneInterfaceWrapper(GetScene()));
}

namespace {
struct EdgeData {
  uint8_t edgeType;
  uint8_t edgeIndex;
  std::optional<uint32> mateVertexIndex;
  uint32 triangleIndex;
  std::array<uint32, 3> triangleVertices;
};

void buildTriangleAdjacency() {
  // TODO is this really needed? can we reuse chaos or mikktspace?
}

struct VisibleEdgeResult {
  std::vector<uint32> edgeIndices;
  std::vector<EdgeData> edgeData;
  TSet<uint32> seenEdgeHashes;
};

/**
 * @brief Parse the EXT_mesh_primitive_edge_visibility 2-bit edge encoding and
 * extract a unique set of edges that should be considered for rendering. The
 * possible are edge types:
 *
 *
 * @param
 */
VisibleEdgeResult extractVisibleEdges(
    const CesiumGltf::Model& model,
    const CesiumGltf::ExtensionExtMeshPrimitiveEdgeVisibility& edgeExtension,
    FRawStaticIndexBuffer& meshIndices,
    uint32 vertexCount) {
  VisibleEdgeResult result;
  CesiumGltf::AccessorView<uint8_t> visibilityAccessor(
      model,
      edgeExtension.visibility);
  if (visibilityAccessor.status() != CesiumGltf::AccessorViewStatus::Valid) {
    // TODO: LOG
    return result;
  }

  const int32 indexCount = meshIndices.GetNumIndices();
  int32 silhouetteEdgeCount = 0;
  int64 edgeIndex = 0;

  // Iterate over triangles (every three indices).
  for (int32 i = 0; i + 2 < indexCount; i += 3) {
    uint32 v0 = meshIndices.GetIndex(i);
    uint32 v1 = meshIndices.GetIndex(i + 1);
    uint32 v2 = meshIndices.GetIndex(i + 2);

    for (uint8 e = 0; e < 3; e++) {
      // Identify the next edge in the triangle.
      // For each triangle (v0, v1, v2), the bitfield encodes three visibility
      // values for the edges (v0:v1, v1:v2, v2:v0) in that order.
      uint32 a = 0, b = 0;
      if (e == 0) {
        a = v0;
        b = v1;
      } else if (e == 1) {
        a = v1;
        b = v2;
      } else if (e == 2) {
        a = v2;
        b = v0;
      }

      // Get the corresponding visibility value for the edge.
      uint32 byteIndex = FMath::Floor(edgeIndex / 4);
      uint32 bitPairOffset = (edgeIndex % 4) * 2;
      edgeIndex++;

      if (byteIndex >= visibilityAccessor.size()) {
        break;
      }

      // Get the edge type encoded as two bits.
      uint8_t byte = visibilityAccessor[byteIndex];
      uint8_t edgeType = (byte >> bitPairOffset) & 0x3;

      if (edgeType !=
          ExtensionExtMeshPrimitiveEdgeVisibility::Visibility::HIDDEN) {
        const uint32 smallestIndex = FMath::Min(a, b);
        const uint32 biggestIndex = FMath::Max(a, b);
        const uint32 hash = smallestIndex * vertexCount + biggestIndex;

        if (!result.seenEdgeHashes.Contains(hash)) {
          // Create the edge if it hasn't already been generated.
          result.seenEdgeHashes.Add(hash);
          result.edgeIndices.emplace_back(a);
          result.edgeIndices.emplace_back(b);

          std::optional<uint32> mateVertexIndex;
          if (edgeType ==
              ExtensionExtMeshPrimitiveEdgeVisibility::Visibility::SILHOUETTE) {
            // TODO: what is mate vertex index???
            mateVertexIndex = silhouetteEdgeCount;
            silhouetteEdgeCount++;
          }

          result.edgeData.emplace_back(EdgeData{
              .edgeType = edgeType,
              .edgeIndex = e,
              .mateVertexIndex = mateVertexIndex,
              .triangleIndex = uint32(i / 3),
              .triangleVertices = {v0, v1, v2}});
        }
      }
    }
  }
  return result;
}

// TODO this is inevitably altered by our Unreal code..
// pass the actual position/index buffer in :')
void createEdgeGeometry(
    const CesiumGltf::Model& model,
    const CesiumGltf::ExtensionExtMeshPrimitiveEdgeVisibility& edgeExtension,
    const VisibleEdgeResult& visibleEdges,
    const FPositionVertexBuffer& meshPositions,
    FStaticMeshLODResources& edgeResources) {
  if (visibleEdges.edgeIndices.size() == 0) {
    return;
  }
  const uint32 vertexCount = meshPositions.GetNumVertices();

  // Needed for SILHOUETTE edges, if present.
  // TODO: supported types are float, byte, short.
  CesiumGltf::AccessorView<glm::vec3> silhouetteNormalAccessor(
      model,
      edgeExtension.silhouetteNormals);
  bool hasSilhouetteNormals = silhouetteNormalAccessor.status() ==
                              CesiumGltf::AccessorViewStatus::Valid;

  size_t generatedEdgeCount = visibleEdges.edgeData.size();
  uint32 totalEdgeVertices = generatedEdgeCount * 2;

  FPositionVertexBuffer& edgePositions =
      edgeResources.VertexBuffers.PositionVertexBuffer;
  edgePositions.Init(totalEdgeVertices, false);

  FStaticMeshVertexBuffer& edgeVertexBuffer =
      edgeResources.VertexBuffers.StaticMeshVertexBuffer;
  // Edge type is passed through as a UV coordinate.
  edgeVertexBuffer.Init(totalEdgeVertices, 1, false);

  uint32 vertexIndex = 0;
  for (size_t e = 0; e < generatedEdgeCount; e++) {
    FVector3f& outPosition0 = edgePositions.VertexPosition(vertexIndex++);
    FVector3f& outPosition1 = edgePositions.VertexPosition(vertexIndex++);

    // Get the edge endpoints (as indices into the original mesh).
    const uint32 a = visibleEdges.edgeIndices[e * 2];
    const uint32 b = visibleEdges.edgeIndices[e * 2 + 1];

    if (a < 0 || b < 0 || a >= vertexCount || b >= vertexCount) {
      // If the indices are invalid, fill with default values.
      outPosition0 = FVector3f::Zero();
      outPosition1 = FVector3f::Zero();
      edgeVertexBuffer.SetVertexTangents(
          e * 2,
          FVector3f(0, 0, 1),
          FVector3f(0, 0, 1),
          FVector3f(0, 0, 1));
      edgeVertexBuffer.SetVertexTangents(
          e * 2 + 1,
          FVector3f(0, 0, 1),
          FVector3f(0, 0, 1),
          FVector3f(0, 0, 1));
      continue;
    }

    const FVector3f& positionA = meshPositions.VertexPosition(a);
    const FVector3f& positionB = meshPositions.VertexPosition(b);

    outPosition0 = positionA;
    outPosition1 = positionB;

    const uint8_t edgeType = visibleEdges.edgeData[e].edgeType;
    // does this really have to be encoded this way?
    float t = float(edgeType) / 255.0f;

    edgeVertexBuffer.SetVertexUV(e * 2, 0, FVector2f(t, 0.0f));
    edgeVertexBuffer.SetVertexUV(e * 2 + 1, 0, FVector2f(t, 0.0f));

    // Add silhouette normal for silhouette edges (type 1)
    // tangentX = silhouette normal
    // tangentY = faceNormalA
    // tangentZ = faceNormalB
    FVector3f silhouetteNormal(0, 0, 1);

    if (edgeType ==
            ExtensionExtMeshPrimitiveEdgeVisibility::Visibility::SILHOUETTE &&
        hasSilhouetteNormals) {
      uint32 mateVertexIndex =
          visibleEdges.edgeData[e].mateVertexIndex.value_or(0);
      if (mateVertexIndex < silhouetteNormalAccessor.size()) {
        // silhouetteNormal = silhouetteNormalAccessor[mateVertexIndex];
      }
    }
    FVector3f faceNormalA(0, 0, 1);
    FVector3f faceNormalB(0, 0, 1);

    edgeVertexBuffer
        .SetVertexTangents(e, silhouetteNormal, faceNormalA, faceNormalB);
    edgeVertexBuffer
        .SetVertexTangents(e + 1, silhouetteNormal, faceNormalA, faceNormalB);
  }
}
} // namespace

FCesiumMeshPrimitiveEdgeData::FCesiumMeshPrimitiveEdgeData(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& meshPrimitive,
    const CesiumGltf::ExtensionExtMeshPrimitiveEdgeVisibility& extension,
    LoadGltfResult::LoadedPrimitiveResult& primitiveResult) {
  if (!primitiveResult.pRenderData ||
      primitiveResult.pRenderData->LODResources.IsEmpty()) {
    return;
  }

  FStaticMeshLODResources& lodResources =
      primitiveResult.pRenderData->LODResources[0];
  FRawStaticIndexBuffer& indices = lodResources.IndexBuffer;
  FPositionVertexBuffer& positionBuffer =
      lodResources.VertexBuffers.PositionVertexBuffer;
  const int32 indexCount = indices.GetNumIndices();
  const uint32 vertexCount = positionBuffer.GetNumVertices();

  VisibleEdgeResult visibleEdges =
      extractVisibleEdges(model, extension, indices, vertexCount);

  // buildTriangleAdjacency
  // generateFaceEdgeNormals: reuse existing?

  this->pEdgeRenderData = MakeUnique<FStaticMeshRenderData>();
  this->pEdgeRenderData->AllocateLODResources(1);

  FStaticMeshLODResources& edgeResources =
      this->pEdgeRenderData->LODResources[0];
  createEdgeGeometry(
      model,
      extension,
      visibleEdges,
      positionBuffer,
      edgeResources);
}
