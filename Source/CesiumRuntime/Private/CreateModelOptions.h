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

  CreateNodeOptions() = default;

  CreateNodeOptions(const CreateModelOptions& modelOptions_)
      : pModelOptions(&modelOptions_) {}
};

struct CreateMeshOptions {
  const CreateNodeOptions* pNodeOptions = nullptr;
  const CesiumGltf::Mesh* pMesh = nullptr;

  CreateMeshOptions() = default;

  CreateMeshOptions(const CreateNodeOptions& nodeOptions_)
      : pNodeOptions(&nodeOptions_) {}
};

struct CreatePrimitiveOptions {
  const CreateMeshOptions* pMeshOptions = nullptr;
  const CesiumGltf::MeshPrimitive* pPrimitive = nullptr;

  CreatePrimitiveOptions() = default;

  CreatePrimitiveOptions(const CreateMeshOptions& meshOptions_)
      : pMeshOptions(&meshOptions_) {}
};
