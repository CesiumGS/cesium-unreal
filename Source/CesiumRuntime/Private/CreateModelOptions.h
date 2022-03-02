// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

struct CreateModelOptions {
  const CesiumGltf::Model* pModel = nullptr;
  bool alwaysIncludeTangents = false;
#if PHYSICS_INTERFACE_PHYSX
  IPhysXCooking* pPhysXCooking = nullptr;
#endif
};

struct CreateNodeOptions {
  const CreateModelOptions* pModelOptions = nullptr;
  const CesiumGltf::Node* pNode = nullptr;
};

struct CreateMeshOptions {
  const CreateNodeOptions* pNodeOptions = nullptr;
  const CesiumGltf::Mesh* pMesh = nullptr;
  std::vector<int32> variants{};
};

struct CreatePrimitiveOptions {
  const CreateMeshOptions* pMeshOptions = nullptr;
  const CesiumGltf::MeshPrimitive* pPrimitive = nullptr;
};
