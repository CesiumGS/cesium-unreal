// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

struct CreateModelOptions {
  bool alwaysIncludeTangents = false;
#if PHYSICS_INTERFACE_PHYSX
  IPhysXCooking* pPhysXCooking = nullptr;
#endif
};

struct CreateNodeOptions {
  CreateModelOptions modelOptions{};

  CreateNodeOptions(const CreateModelOptions& modelOptions_) 
    : modelOptions(modelOptions_) {}
};

struct CreateMeshOptions {
  CreateNodeOptions nodeOptions{};
  std::vector<int32> variants{};

  CreateMeshOptions(const CreateNodeOptions& nodeOptions_) 
    : nodeOptions(nodeOptions_) {}
};

struct CreatePrimitiveOptions {
  CreateMeshOptions meshOptions{};
  
  CreatePrimitiveOptions(const CreateMeshOptions& meshOptions_) 
    : meshOptions(meshOptions_) {}
};
