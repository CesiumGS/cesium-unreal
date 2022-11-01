// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumEncodedMetadataComponent.h"
#include "CesiumGltf/Mesh.h"
#include "CesiumGltf/MeshPrimitive.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/Node.h"
#include "LoadGltfResult.h"
#if PHYSICS_INTERFACE_PHYSX
#include "IPhysXCookingModule.h"
#include <PxTriangleMesh.h>
#endif

#include <gsl/span>

#include <cstdint>
#include <memory>
#include <vector>

// TODO: internal documentation
namespace CreateGltfOptions {
struct CreateModelOptions {
  CesiumGltf::Model* pModel = nullptr;
  gsl::span<const std::byte> derivedDataCache{};
  std::vector<gsl::span<const std::byte>> preCookedPhysicsMeshes{};
  const FMetadataDescription* pEncodedMetadataDescription = nullptr;
  bool alwaysIncludeTangents = false;
#if PHYSICS_INTERFACE_PHYSX
  IPhysXCookingModule* pPhysXCookingModule = nullptr;
#endif
};

struct CreateNodeOptions {
  const CreateModelOptions* pModelOptions = nullptr;
  const LoadGltfResult::LoadModelResult* pHalfConstructedModelResult = nullptr;
  const CesiumGltf::Node* pNode = nullptr;
};

struct CreateMeshOptions {
  const CreateNodeOptions* pNodeOptions = nullptr;
  const LoadGltfResult::LoadNodeResult* pHalfConstructedNodeResult = nullptr;
  const CesiumGltf::Mesh* pMesh = nullptr;
};

struct CreatePrimitiveOptions {
  uint32_t primitiveIndex = 0;
  const CreateMeshOptions* pMeshOptions = nullptr;
  const LoadGltfResult::LoadMeshResult* pHalfConstructedMeshResult = nullptr;
  const CesiumGltf::MeshPrimitive* pPrimitive = nullptr;
};
} // namespace CreateGltfOptions
