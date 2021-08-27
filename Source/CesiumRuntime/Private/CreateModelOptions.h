// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

struct CreateModelOptions {
  bool alwaysIncludeTangents = false;
#if PHYSICS_INTERFACE_PHYSX
  IPhysXCooking* pPhysXCooking = nullptr;
#endif
};
