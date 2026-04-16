#include "CgalGltfModifier.h"

#include <Cesium3DTilesSelection/TilesetMetadata.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/ExtensionCesiumRTC.h>
#include <CesiumGltf/Model.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cstring>
#include <limits>
#include <vector>

#include "CgalMeshOperations.h"

#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "StaticMeshResources.h"

CgalGltfModifier::CgalGltfModifier() = default;
CgalGltfModifier::~CgalGltfModifier() = default;

void CgalGltfModifier::SetStaticMeshActor(AStaticMeshActor* InStaticMeshActor) {
  StaticMeshActor = InStaticMeshActor;
}

AStaticMeshActor* CgalGltfModifier::GetStaticMeshActor() const {
  return StaticMeshActor;
}

void CgalGltfModifier::CacheClipMeshData(const glm::dmat4& unrealToEcef) {
  ClipMeshPositionsECEF.clear();
  ClipMeshIndices.clear();
  bClipMeshReady = false;

  if (!StaticMeshActor) {
    UE_LOG(LogTemp, Warning, TEXT("CgalGltfModifier::CacheClipMeshData — no StaticMeshActor"));
    return;
  }

  UStaticMeshComponent* MeshComp = StaticMeshActor->GetStaticMeshComponent();
  if (!MeshComp || !MeshComp->GetStaticMesh()) {
    UE_LOG(LogTemp, Warning, TEXT("CgalGltfModifier::CacheClipMeshData — no static mesh"));
    return;
  }

  UStaticMesh* Mesh = MeshComp->GetStaticMesh();
  const FStaticMeshRenderData* RenderData = Mesh->GetRenderData();
  if (!RenderData || RenderData->LODResources.Num() == 0) {
    UE_LOG(LogTemp, Warning, TEXT("CgalGltfModifier::CacheClipMeshData — no render data"));
    return;
  }

  const FStaticMeshLODResources& LOD = RenderData->LODResources[0];
  const FPositionVertexBuffer& PosBuf = LOD.VertexBuffers.PositionVertexBuffer;
  int32 NumVertices = PosBuf.GetNumVertices();
  int32 NumIndices = LOD.IndexBuffer.GetNumIndices();

  if (NumVertices == 0 || NumIndices < 3) {
    UE_LOG(LogTemp, Warning, TEXT("CgalGltfModifier::CacheClipMeshData — empty mesh"));
    return;
  }

  // Build the combined transform: mesh-local → Unreal world → ECEF.
  FTransform ActorTransform = StaticMeshActor->GetActorTransform();
  glm::dmat4 actorMatrix(
      glm::dvec4(ActorTransform.ToMatrixWithScale().M[0][0],
                  ActorTransform.ToMatrixWithScale().M[0][1],
                  ActorTransform.ToMatrixWithScale().M[0][2],
                  ActorTransform.ToMatrixWithScale().M[0][3]),
      glm::dvec4(ActorTransform.ToMatrixWithScale().M[1][0],
                  ActorTransform.ToMatrixWithScale().M[1][1],
                  ActorTransform.ToMatrixWithScale().M[1][2],
                  ActorTransform.ToMatrixWithScale().M[1][3]),
      glm::dvec4(ActorTransform.ToMatrixWithScale().M[2][0],
                  ActorTransform.ToMatrixWithScale().M[2][1],
                  ActorTransform.ToMatrixWithScale().M[2][2],
                  ActorTransform.ToMatrixWithScale().M[2][3]),
      glm::dvec4(ActorTransform.ToMatrixWithScale().M[3][0],
                  ActorTransform.ToMatrixWithScale().M[3][1],
                  ActorTransform.ToMatrixWithScale().M[3][2],
                  ActorTransform.ToMatrixWithScale().M[3][3]));

  glm::dmat4 toEcef = unrealToEcef * actorMatrix;

  // Transform vertices to ECEF and cache.
  ClipMeshPositionsECEF.resize(NumVertices);
  for (int32 i = 0; i < NumVertices; ++i) {
    const FVector3f& Pos = PosBuf.VertexPosition(i);
    glm::dvec4 ecef =
        toEcef * glm::dvec4(Pos.X, Pos.Y, Pos.Z, 1.0);
    ClipMeshPositionsECEF[i] = {
        static_cast<float>(ecef.x),
        static_cast<float>(ecef.y),
        static_cast<float>(ecef.z)};
  }

  // Cache indices.
  ClipMeshIndices.resize(NumIndices);
  for (int32 i = 0; i < NumIndices; ++i) {
    ClipMeshIndices[i] = LOD.IndexBuffer.GetIndex(i);
  }

  bClipMeshReady = true;

  UE_LOG(
      LogTemp,
      Log,
      TEXT("CgalGltfModifier::CacheClipMeshData — cached %d vertices, %d "
           "indices (%d triangles)"),
      NumVertices,
      NumIndices,
      NumIndices / 3);
}

CesiumAsync::Future<void> CgalGltfModifier::onRegister(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const Cesium3DTilesSelection::TilesetMetadata& tilesetMetadata,
    const Cesium3DTilesSelection::Tile& rootTile) {
  return asyncSystem.createResolvedFuture();
}


namespace {

// Read all indices from a primitive's index accessor as uint32.
std::vector<uint32_t> readIndices(
    const CesiumGltf::Model& model,
    const CesiumGltf::Accessor& accessor) {
  std::vector<uint32_t> result(accessor.count);
  if (accessor.bufferView < 0)
    return result;

  const auto& bv = model.bufferViews[accessor.bufferView];
  const auto& buf = model.buffers[bv.buffer];
  const std::byte* base =
      buf.cesium.data.data() + bv.byteOffset + accessor.byteOffset;

  switch (accessor.componentType) {
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE:
    for (int64_t i = 0; i < accessor.count; ++i)
      result[i] = reinterpret_cast<const uint8_t*>(base)[i];
    break;
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT:
    for (int64_t i = 0; i < accessor.count; ++i)
      result[i] = reinterpret_cast<const uint16_t*>(base)[i];
    break;
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_INT:
    for (int64_t i = 0; i < accessor.count; ++i)
      result[i] = reinterpret_cast<const uint32_t*>(base)[i];
    break;
  default:
    break;
  }
  return result;
}

// Read positions from a POSITION accessor as float triples.
std::vector<std::array<float, 3>> readPositions(
    const CesiumGltf::Model& model,
    const CesiumGltf::Accessor& accessor) {
  std::vector<std::array<float, 3>> result(accessor.count);
  if (accessor.bufferView < 0)
    return result;

  const auto& bv = model.bufferViews[accessor.bufferView];
  const auto& buf = model.buffers[bv.buffer];
  const std::byte* base =
      buf.cesium.data.data() + bv.byteOffset + accessor.byteOffset;
  int64_t stride = accessor.computeByteStride(model);

  for (int64_t i = 0; i < accessor.count; ++i) {
    const float* f =
        reinterpret_cast<const float*>(base + i * stride);
    result[i] = {f[0], f[1], f[2]};
  }
  return result;
}

} // anonymous namespace

bool isIdentityMatrix(glm::dmat4 matrix) {
  int numZeros=0;
  int numOnes =0;

  const double* pData = (const double*)glm::value_ptr(matrix);

  for (int i=0; i < 16; ++i) {
    if (pData[i] == 0.0) { ++numZeros; }
    else if (pData[i] == 1.0) { ++numOnes; }
  }

  return numZeros + numOnes == 16;
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

  if (bClipMeshReady && !ClipMeshPositionsECEF.empty()) {
    // Build the full mesh-local-to-ECEF transform.
    // Start with the tile transform (already includes parent transforms).
    glm::dmat4 meshToEcef = input.tileTransform;

    // Check for CESIUM_RTC extension (used by B3DM/PNTS).
    const CesiumGltf::ExtensionCesiumRTC* pRTC =
        modifiedModel.getExtension<CesiumGltf::ExtensionCesiumRTC>();
    if (pRTC && pRTC->center.size() == 3) {
      glm::dmat4 rtcTransform(
          glm::dvec4(1.0, 0.0, 0.0, 0.0),
          glm::dvec4(0.0, 1.0, 0.0, 0.0),
          glm::dvec4(0.0, 0.0, 1.0, 0.0),
          glm::dvec4(pRTC->center[0], pRTC->center[1], pRTC->center[2], 1.0));
      meshToEcef = meshToEcef * rtcTransform;
      UE_LOG(
          LogTemp, Log,
          TEXT("CgalGltfModifier::apply #%d — RTC center=(%.2f, %.2f, %.2f)"),
          invocations,
          pRTC->center[0], pRTC->center[1], pRTC->center[2]);
    }

    // Check for node transforms (used by quantized mesh terrain).
    // Walk the scene graph: scene -> nodes -> accumulate transforms.
    // For simplicity, find the root node that contains the mesh and
    // apply its matrix.
    for (const auto& node : modifiedModel.nodes) {
      if (node.mesh >= 0 && node.matrix.size() == 16) {
        // glTF stores matrices column-major.
        const auto& m = node.matrix;
        glm::dmat4 nodeTransform(
            glm::dvec4(m[0], m[1], m[2], m[3]),
            glm::dvec4(m[4], m[5], m[6], m[7]),
            glm::dvec4(m[8], m[9], m[10], m[11]),
            glm::dvec4(m[12], m[13], m[14], m[15]));
        // Check if it's non-identity (has a meaningful transform).
        if (nodeTransform != glm::dmat4(1.0)) {
          meshToEcef = meshToEcef * nodeTransform;
          UE_LOG(
              LogTemp, Log,
              TEXT("CgalGltfModifier::apply #%d — node transform "
                   "translation=(%.2f, %.2f, %.2f)"),
              invocations, m[12], m[13], m[14]);
          break; // Use the first mesh node's transform.
        }
      }
    }

    glm::dmat4 ecefToMeshLocal = glm::inverse(meshToEcef);

    std::vector<std::array<float, 3>> clipPositionsTileLocal(
        ClipMeshPositionsECEF.size());
    for (size_t i = 0; i < ClipMeshPositionsECEF.size(); ++i) {
      const auto& p = ClipMeshPositionsECEF[i];
      // glm::dvec4 tileLocal =
      //     ecefToMeshLocal * glm::dvec4(p[0], p[1], p[2], 1.0);
      glm::dvec4 tileLocal = {
        p[0] + ecefToMeshLocal[3][0],
        p[2] + ecefToMeshLocal[3][2],
        p[1] + ecefToMeshLocal[3][1],
        1.0
      };
      clipPositionsTileLocal[i] = {
          static_cast<float>(tileLocal.x),
          static_cast<float>(tileLocal.y),
          static_cast<float>(tileLocal.z)};
    }

    // Compute AABB of the clip mesh in tile-local space for fast rejection.
    float clipMin[3] = {
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()};
    float clipMax[3] = {
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest()};
    for (const auto& p : clipPositionsTileLocal) {
      for (int c = 0; c < 3; ++c) {
        clipMin[c] = std::min(clipMin[c], p[c]);
        clipMax[c] = std::max(clipMax[c], p[c]);
      }
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("CgalGltfModifier::apply #%d — clip AABB (ECEF/tile-local): "
             "min=(%.2f, %.2f, %.2f) max=(%.2f, %.2f, %.2f)"),
        invocations,
        clipMin[0], clipMin[1], clipMin[2],
        clipMax[0], clipMax[1], clipMax[2]);

    // Log the first tile primitive's AABB so we can compare coordinate spaces.
    for (const auto& mesh : modifiedModel.meshes) {
      for (const auto& prim : mesh.primitives) {
        auto posIt = prim.attributes.find("POSITION");
        if (posIt == prim.attributes.end())
          continue;
        int32_t posIdx = posIt->second;
        if (posIdx < 0 ||
            posIdx >= static_cast<int32_t>(modifiedModel.accessors.size()))
          continue;
        const auto& acc = modifiedModel.accessors[posIdx];
        if (acc.min.size() >= 3 && acc.max.size() >= 3) {
          UE_LOG(
              LogTemp,
              Log,
              TEXT("CgalGltfModifier::apply #%d — tile primitive AABB: "
                   "min=(%.2f, %.2f, %.2f) max=(%.2f, %.2f, %.2f)"),
              invocations,
              acc.min[0], acc.min[1], acc.min[2],
              acc.max[0], acc.max[1], acc.max[2]);
          goto done_logging_tile_aabb;
        }
      }
    }
    done_logging_tile_aabb:

    // Debug clipping: remove any triangle whose AABB overlaps the clip
    // mesh's AABB (simple but effective for debugging the transform chain).
    for (auto& mesh : modifiedModel.meshes) {
      for (auto& prim : mesh.primitives) {
        if (prim.mode != CesiumGltf::MeshPrimitive::Mode::TRIANGLES)
          continue;
        if (prim.indices < 0 ||
            prim.indices >=
                static_cast<int32_t>(modifiedModel.accessors.size()))
          continue;

        auto posIt = prim.attributes.find("POSITION");
        if (posIt == prim.attributes.end())
          continue;
        int32_t posAccessorIdx = posIt->second;
        if (posAccessorIdx < 0 ||
            posAccessorIdx >=
                static_cast<int32_t>(modifiedModel.accessors.size()))
          continue;

        auto& idxAccessor = modifiedModel.accessors[prim.indices];
        auto& posAccessor = modifiedModel.accessors[posAccessorIdx];

        if (posAccessor.count == 0 || idxAccessor.count < 3)
          continue;

        auto tilePositions = readPositions(modifiedModel, posAccessor);
        auto tileIndices = readIndices(modifiedModel, idxAccessor);

        // Keep only triangles whose AABB does NOT overlap the clip AABB.
        std::vector<uint32_t> keptIndices;
        keptIndices.reserve(tileIndices.size());
        int64_t removedCount = 0;

        for (size_t t = 0; t + 2 < tileIndices.size(); t += 3) {
          uint32_t i0 = tileIndices[t];
          uint32_t i1 = tileIndices[t + 1];
          uint32_t i2 = tileIndices[t + 2];

          // Compute triangle AABB.
          float triMin[3], triMax[3];
          for (int c = 0; c < 3; ++c) {
            triMin[c] = std::min({
                tilePositions[i0][c],
                tilePositions[i1][c],
                tilePositions[i2][c]});
            triMax[c] = std::max({
                tilePositions[i0][c],
                tilePositions[i1][c],
                tilePositions[i2][c]});
          }

          // Test AABB overlap.
          bool overlaps = true;
          for (int c = 0; c < 3; ++c) {
            if (triMax[c] < clipMin[c] || triMin[c] > clipMax[c]) {
              overlaps = false;
              break;
            }
          }

          if (!overlaps) {
            keptIndices.push_back(i0);
            keptIndices.push_back(i1);
            keptIndices.push_back(i2);
          } else {
            ++removedCount;
          }
        }

        UE_LOG(
            LogTemp,
            Log,
            TEXT("CgalGltfModifier::apply #%d — primitive: %lld triangles, "
                 "removed %lld, kept %lld"),
            invocations,
            static_cast<long long>(tileIndices.size() / 3),
            static_cast<long long>(removedCount),
            static_cast<long long>(keptIndices.size() / 3));

        // Write kept indices into a new buffer.
        int64_t newIdxCount = static_cast<int64_t>(keptIndices.size());
        std::vector<std::byte> newIdxData(newIdxCount * sizeof(uint32_t));
        std::memcpy(newIdxData.data(), keptIndices.data(), newIdxData.size());

        int32_t newIdxBufIdx =
            static_cast<int32_t>(modifiedModel.buffers.size());
        auto& newIdxBuf = modifiedModel.buffers.emplace_back();
        newIdxBuf.cesium.data = std::move(newIdxData);
        newIdxBuf.byteLength =
            static_cast<int64_t>(newIdxBuf.cesium.data.size());

        int32_t newIdxBvIdx =
            static_cast<int32_t>(modifiedModel.bufferViews.size());
        auto& newIdxBv = modifiedModel.bufferViews.emplace_back();
        newIdxBv.buffer = newIdxBufIdx;
        newIdxBv.byteOffset = 0;
        newIdxBv.byteLength = newIdxBuf.byteLength;

        idxAccessor.bufferView = newIdxBvIdx;
        idxAccessor.byteOffset = 0;
        idxAccessor.count = newIdxCount;
        idxAccessor.componentType =
            CesiumGltf::Accessor::ComponentType::UNSIGNED_INT;
      }
    }
  }

#if 0 // CGAL boolean clipping — disabled for debugging
  if (bClipMeshReady && !ClipMeshPositionsECEF.empty()) {
    glm::dmat4 ecefToMeshLocal = glm::inverse(input.tileTransform);
    std::vector<std::array<float, 3>> clipPositionsTileLocal(
        ClipMeshPositionsECEF.size());
    for (size_t i = 0; i < ClipMeshPositionsECEF.size(); ++i) {
      const auto& p = ClipMeshPositionsECEF[i];
      glm::dvec4 tileLocal =
          ecefToMeshLocal * glm::dvec4(p[0], p[1], p[2], 1.0);
      clipPositionsTileLocal[i] = {
          static_cast<float>(tileLocal.x),
          static_cast<float>(tileLocal.y),
          static_cast<float>(tileLocal.z)};
    }
    for (auto& mesh : modifiedModel.meshes) {
      for (auto& prim : mesh.primitives) {
        if (prim.mode != CesiumGltf::MeshPrimitive::Mode::TRIANGLES)
          continue;
        if (prim.indices < 0 ||
            prim.indices >=
                static_cast<int32_t>(modifiedModel.accessors.size()))
          continue;
        auto posIt = prim.attributes.find("POSITION");
        if (posIt == prim.attributes.end())
          continue;
        int32_t posAccessorIdx = posIt->second;
        if (posAccessorIdx < 0 ||
            posAccessorIdx >=
                static_cast<int32_t>(modifiedModel.accessors.size()))
          continue;
        auto& idxAccessor = modifiedModel.accessors[prim.indices];
        auto& posAccessor = modifiedModel.accessors[posAccessorIdx];
        if (posAccessor.count == 0 || idxAccessor.count < 3)
          continue;
        auto tilePositions = readPositions(modifiedModel, posAccessor);
        auto tileIndices = readIndices(modifiedModel, idxAccessor);
        std::vector<std::array<float, 3>> outPositions;
        std::vector<uint32_t> outIndices;
        bool ok = CesiumCgal::BooleanOperation(
            tilePositions, tileIndices,
            clipPositionsTileLocal, ClipMeshIndices,
            CesiumCgal::BooleanOp::Difference,
            outPositions, outIndices);
        if (!ok) continue;
        // ... write back results ...
      }
    }
  }
#endif

  int64_t outputTriangles = countTriangles(modifiedModel);

  // Compute overall bounding box from POSITION accessor min/max bounds.
  double bboxMin[3] = {
      std::numeric_limits<double>::max(),
      std::numeric_limits<double>::max(),
      std::numeric_limits<double>::max()};
  double bboxMax[3] = {
      std::numeric_limits<double>::lowest(),
      std::numeric_limits<double>::lowest(),
      std::numeric_limits<double>::lowest()};
  bool hasBounds = false;
  for (const auto& mesh : modifiedModel.meshes) {
    for (const auto& prim : mesh.primitives) {
      auto posIt = prim.attributes.find("POSITION");
      if (posIt == prim.attributes.end())
        continue;
      int32_t posIdx = posIt->second;
      if (posIdx < 0 ||
          posIdx >= static_cast<int32_t>(modifiedModel.accessors.size()))
        continue;
      const auto& acc = modifiedModel.accessors[posIdx];
      if (acc.min.size() >= 3 && acc.max.size() >= 3) {
        for (int i = 0; i < 3; ++i) {
          bboxMin[i] = std::min(bboxMin[i], acc.min[i]);
          bboxMax[i] = std::max(bboxMax[i], acc.max[i]);
        }
        hasBounds = true;
      }
    }
  }
  double cx = 0.0, cy = 0.0, cz = 0.0;
  double ex = 0.0, ey = 0.0, ez = 0.0;
  if (hasBounds) {
    cx = (bboxMin[0] + bboxMax[0]) * 0.5;
    cy = (bboxMin[1] + bboxMax[1]) * 0.5;
    cz = (bboxMin[2] + bboxMax[2]) * 0.5;
    ex = bboxMax[0] - bboxMin[0];
    ey = bboxMax[1] - bboxMin[1];
    ez = bboxMax[2] - bboxMin[2];
  }

  double distanceToOrigin = glm::length(glm::dvec3(cx, cy, cz));

  UE_LOG(
      LogTemp,
      Log,
      TEXT("CgalGltfModifier::apply #%d — input triangles=%lld, "
           "output triangles=%lld, center=(%.2f, %.2f, %.2f), "
           "extents=(%.2f, %.2f, %.2f), distanceToOrigin=(%.2f)"),
      invocations,
      static_cast<long long>(inputTriangles),
      static_cast<long long>(outputTriangles),
      cx, cy, cz,
      ex, ey, ez,
      distanceToOrigin);

  Cesium3DTilesSelection::GltfModifierOutput output;
  output.modifiedModel = std::move(modifiedModel);

  return input.asyncSystem
      .createResolvedFuture<
          std::optional<Cesium3DTilesSelection::GltfModifierOutput>>(
          std::move(output));
}
