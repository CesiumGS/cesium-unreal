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
#endif

// TODO: internal documentation
namespace CreateGltfOptions {
struct CreateModelOptions {
  CesiumGltf::Model* pModel = nullptr;
  const FMetadataDescription* pEncodedMetadataDescription = nullptr;
  bool alwaysIncludeTangents = false;
  bool createPhysicsMeshes = true;
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
  const CreateMeshOptions* pMeshOptions = nullptr;
  const LoadGltfResult::LoadMeshResult* pHalfConstructedMeshResult = nullptr;
  const CesiumGltf::MeshPrimitive* pPrimitive = nullptr;
};
} // namespace CreateGltfOptions
