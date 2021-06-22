// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

struct CESIUMRUNTIME_API CreateModelOptions {
  bool alwaysIncludeTangents = false;
#if PHYSICS_INTERFACE_PHYSX
  IPhysXCooking* pPhysXCooking = nullptr;
#endif
};
