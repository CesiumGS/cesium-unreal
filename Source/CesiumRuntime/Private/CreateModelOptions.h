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
  CreateModelOptions modelOptions{};
  const CesiumGltf::Node* pNode = nullptr;

  CreateNodeOptions() = default;

  CreateNodeOptions(const CreateModelOptions& modelOptions_)
      : modelOptions(modelOptions_) {}
};

struct CreateMeshOptions {
  CreateNodeOptions nodeOptions{};
  const CesiumGltf::Mesh* pMesh = nullptr;

  CreateMeshOptions() = default;

  CreateMeshOptions(const CreateNodeOptions& nodeOptions_)
      : nodeOptions(nodeOptions_) {}
};

struct CreatePrimitiveOptions {
  CreateMeshOptions meshOptions{};
  const CesiumGltf::MeshPrimitive* pPrimitive = nullptr;

  CreatePrimitiveOptions() = default;

  CreatePrimitiveOptions(const CreateMeshOptions& meshOptions_)
      : meshOptions(meshOptions_) {}
};
