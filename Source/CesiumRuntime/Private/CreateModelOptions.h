// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

struct CreateModelOptions {
  const CesiumGltf::Model* pModel = nullptr;
  bool alwaysIncludeTangents = false;
#if PHYSICS_INTERFACE_PHYSX
  IPhysXCookingModule* pPhysXCookingModule = nullptr;
#endif
};

struct CreateNodeOptions {
  const CreateModelOptions* pModelOptions = nullptr;
  const CesiumGltf::Node* pNode = nullptr;
};

struct CreateMeshOptions {
  const CreateNodeOptions* pNodeOptions = nullptr;
  const CesiumGltf::Mesh* pMesh = nullptr;
};

struct CreatePrimitiveOptions {
  const CreateMeshOptions* pMeshOptions = nullptr;
  const CesiumGltf::MeshPrimitive* pPrimitive = nullptr;
};
