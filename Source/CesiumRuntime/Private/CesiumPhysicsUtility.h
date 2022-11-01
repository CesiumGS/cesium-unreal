// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Containers/Array.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "Templates/SharedPointer.h"
#include "Templates/UniquePtr.h"

#if PHYSICS_INTERFACE_PHYSX
#include "IPhysXCooking.h"
#include "IPhysXCookingModule.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "PhysXCookHelper.h"
#include "PhysicsPublicCore.h"
#include "PxCollection.h"
#include "PxCooking.h"
#include "PxIO.h"
#include "PxPhysics.h"
#include "PxSerialFramework.h"
#include "PxSerialization.h"
#include <PxTriangleMesh.h>
#else
#include "Chaos/AABBTree.h"
#include "Chaos/CollisionConvexMesh.h"
#include "Chaos/TriangleMeshImplicitObject.h"
#endif

#include <gsl/span>

#include <memory>
#include <vector>

namespace CesiumPhysicsUtility {
#if PHYSICS_INTERFACE_PHYSX
struct PxTriangleMeshDeleter {
  void operator()(PxTriangleMesh* pCollisionMesh) {
    if (pCollisionMesh) {
      pCollisionMesh->release();
    }
  }
};

struct CesiumPhysxMesh {
  TUniquePtr<PxTriangleMesh, PxTriangleMeshDeleter> pTriMesh = nullptr;
  FBodySetupUVInfo uvInfo{};
};

CesiumPhysxMesh createPhysxMesh(const gsl::span<const std::byte>& bulkData);
std::vector<std::byte> cookPhysxMesh(
    IPhysXCookingModule* pPhysXCookingModule,
    const TArray<FStaticMeshBuildVertex>& vertexData,
    const TArray<uint32>& indices);
void BuildPhysXTriangleMeshes(
    PxTriangleMesh*& pCollisionMesh,
    FBodySetupUVInfo& uvInfo,
    IPhysXCookingModule* pPhysXCookingModule,
    const TArray<FStaticMeshBuildVertex>& vertexData,
    const TArray<uint32>& indices);
#else
TSharedPtr<Chaos::FTriangleMeshImplicitObject, ESPMode::ThreadSafe>
BuildChaosTriangleMeshes(
    const TArray<FStaticMeshBuildVertex>& vertexData,
    const TArray<uint32>& indices);
#endif
} // namespace CesiumPhysicsUtility
