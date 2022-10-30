// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumPhysicsUtility.h"

#include "PhysicsPublic.h"

namespace CesiumPhysicsUtility {
#if PHYSICS_INTERFACE_PHYSX
namespace {
class CesiumPxInputStream : public PxInputStream {
private:
  gsl::span<const std::byte> _bulkData;
  uint32_t _readPos = 0;

public:
  CesiumPxInputStream(const gsl::span<const std::byte>& bulkData)
      : _bulkData(bulkData) {}

  virtual uint32_t read(void* dest, uint32_t count) override {
    uint32_t bytesToRead = count;
    if (this->_readPos + count > this->_bulkData.size()) {
      bytesToRead =
          static_cast<uint32_t>(this->_bulkData.size() - this->_readPos);
    }

    if (bytesToRead > 0) {
      std::memcpy(dest, (void*)&this->_bulkData[this->_readPos], bytesToRead);
    }

    this->_readPos += bytesToRead;

    return bytesToRead;
  }
};

class CesiumPxOutputStream : public PxOutputStream {
public:
  std::vector<std::byte> bulkData;

  virtual uint32_t write(const void* src, uint32_t count) override {
    if (count > 0) {
      size_t currentSize = this->bulkData.size();
      this->bulkData.resize(currentSize + count);
      std::memcpy((void*)&this->bulkData[currentSize], src, count);
    }

    return count;
  }
};
} // namespace

CesiumPhysxMesh createPhysxMesh(const gsl::span<const std::byte>& bulkData) {
  CesiumPhysxMesh result;

  CesiumPxInputStream inputStream(bulkData);
  result.pTriMesh.Reset(GPhysXSDK->createTriangleMesh(inputStream));
  // TODO: uvs, if needed
  // result.uvInfo.FillFromTriMesh(result.pTriMesh);

  return result;
}

std::vector<std::byte> cookPhysxMesh(
    IPhysXCookingModule* pPhysXCookingModule,
    const TArray<FStaticMeshBuildVertex>& vertexData,
    const TArray<uint32>& indices) {
  if (!pPhysXCookingModule) {
    return {};
  }
  // bool copyUVs = UPhysicsSettings::Get()->bSupportUVFromHitResults;

  PxTriangleMeshDesc mesh;
  mesh.triangles.count = indices.Num() / 3;
  // A "triangle" here is 3 uint32_t indices, so 12 byte stride.
  mesh.triangles.stride = 12;
  mesh.triangles.data = (void*)&indices[0];
  mesh.points.count = vertexData.Num();
  mesh.points.stride = sizeof(FStaticMeshBuildVertex);
  mesh.points.data = (void*)&vertexData[0];
  // Material indices are optional.
  mesh.materialIndices.data = nullptr;
  mesh.materialIndices.stride = 0;

  // Since our meshes switched from a right-handed to
  // left-handed CRS.
  // TODO: ??
  mesh.flags = PxMeshFlag::eFLIPNORMALS;

  // TODO:
  // consider disabling active edge pre-compute, faster cooking, slower
  // contact generation

  PxCooking* pCooking = pPhysXCookingModule->GetPhysXCooking()->GetCooking();
  PxCookingParams oldParams = pCooking->getParams();
  PxCookingParams newParams = oldParams;
  newParams.targetPlatform = PxPlatform::ePC;
  newParams.suppressTriangleMeshRemapTable =
      UPhysicsSettings::Get()->bSuppressFaceRemapTable;
  pCooking->setParams(newParams);

  CesiumPxOutputStream outputStream;
  PxTriangleMeshCookingResult::Enum cookResult;
  pCooking->cookTriangleMesh(mesh, outputStream, &cookResult);
  check(cookResult == PxTriangleMeshCookingResult::eSUCCESS);

  pCooking->setParams(oldParams);
  return std::move(outputStream.bulkData);
}

void BuildPhysXTriangleMeshes(
    PxTriangleMesh*& pCollisionMesh,
    FBodySetupUVInfo& uvInfo,
    IPhysXCookingModule* pPhysXCookingModule,
    const TArray<FStaticMeshBuildVertex>& vertexData,
    const TArray<uint32>& indices) {

  if (pPhysXCookingModule) {
    // TODO: use PhysX interface directly so we don't need to copy the
    // vertices (it takes a stride parameter).

    FPhysXCookHelper cookHelper(pPhysXCookingModule);

    bool copyUVs = UPhysicsSettings::Get()->bSupportUVFromHitResults;

    cookHelper.CookInfo.TriMeshCookFlags = EPhysXMeshCookFlags::Default;
    cookHelper.CookInfo.OuterDebugName = "CesiumGltfComponent";
    cookHelper.CookInfo.TriangleMeshDesc.bFlipNormals = true;
    cookHelper.CookInfo.bCookTriMesh = true;
    cookHelper.CookInfo.bSupportFaceRemap = true;
    cookHelper.CookInfo.bSupportUVFromHitResults = copyUVs;

    TArray<FVector>& vertices = cookHelper.CookInfo.TriangleMeshDesc.Vertices;
    vertices.SetNum(vertexData.Num());
    for (size_t i = 0; i < vertexData.Num(); ++i) {
      vertices[i] = vertexData[i].Position;
    }

    if (copyUVs) {
      TArray<TArray<FVector2D>>& uvs = cookHelper.CookInfo.TriangleMeshDesc.UVs;
      uvs.SetNum(8);

      for (size_t i = 0; i < 8; ++i) {
        uvs[i].SetNum(vertices.Num());
      }
      for (size_t i = 0; i < vertexData.Num(); ++i) {
        for (size_t j = 0; j < 8; ++j) {
          uvs[j][i] = vertexData[i].UVs[j];
        }
      }
    }

    TArray<FTriIndices>& physicsIndices =
        cookHelper.CookInfo.TriangleMeshDesc.Indices;
    physicsIndices.SetNum(indices.Num() / 3);

    for (size_t i = 0; i < indices.Num() / 3; ++i) {
      physicsIndices[i].v0 = indices[3 * i];
      physicsIndices[i].v1 = indices[3 * i + 1];
      physicsIndices[i].v2 = indices[3 * i + 2];
    }

    cookHelper.CreatePhysicsMeshes_Concurrent();
    if (cookHelper.OutTriangleMeshes.Num() > 0) {
      pCollisionMesh = cookHelper.OutTriangleMeshes[0];
    }
    if (copyUVs) {
      uvInfo = std::move(cookHelper.OutUVInfo);
    }
  }
}

#else
template <typename TIndex>
static void fillTriangles(
    TArray<Chaos::TVector<TIndex, 3>>& triangles,
    const TArray<FStaticMeshBuildVertex>& vertexData,
    const TArray<uint32>& indices,
    int32 triangleCount) {

  triangles.Reserve(triangleCount);

  for (int32 i = 0; i < triangleCount; ++i) {
    const int32 index0 = 3 * i;
    triangles.Add(Chaos::TVector<int32, 3>(
        indices[index0 + 1],
        indices[index0],
        indices[index0 + 2]));
  }
}

TSharedPtr<Chaos::FTriangleMeshImplicitObject, ESPMode::ThreadSafe>
BuildChaosTriangleMeshes(
    const TArray<FStaticMeshBuildVertex>& vertexData,
    const TArray<uint32>& indices) {

  int32 vertexCount = vertexData.Num();
  int32 triangleCount = indices.Num() / 3;

  Chaos::TParticles<Chaos::FRealSingle, 3> vertices;
  vertices.AddParticles(vertexCount);

  for (int32 i = 0; i < vertexCount; ++i) {
    vertices.X(i) = vertexData[i].Position;
  }

  TArray<uint16> materials;
  materials.SetNum(triangleCount);

  TArray<int32> faceRemap;
  faceRemap.SetNum(triangleCount);

  for (int32 i = 0; i < triangleCount; ++i) {
    faceRemap[i] = i;
  }

  TUniquePtr<TArray<int32>> pFaceRemap = MakeUnique<TArray<int32>>(faceRemap);

  if (vertexCount < TNumericLimits<uint16>::Max()) {
    TArray<Chaos::TVector<uint16, 3>> triangles;
    fillTriangles(triangles, vertexData, indices, triangleCount);
    return MakeShared<Chaos::FTriangleMeshImplicitObject, ESPMode::ThreadSafe>(
        MoveTemp(vertices),
        MoveTemp(triangles),
        MoveTemp(materials),
        MoveTemp(pFaceRemap),
        nullptr,
        false);
  } else {
    TArray<Chaos::TVector<int32, 3>> triangles;
    fillTriangles(triangles, vertexData, indices, triangleCount);
    return MakeShared<Chaos::FTriangleMeshImplicitObject, ESPMode::ThreadSafe>(
        MoveTemp(vertices),
        MoveTemp(triangles),
        MoveTemp(materials),
        MoveTemp(pFaceRemap),
        nullptr,
        false);
  }
}
#endif
} // namespace CesiumPhysicsUtility
