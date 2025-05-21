// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumRuntime.h"

class CESIUMRUNTIME_API ICesiumObjectAtRelativeHeight {
public:
  virtual void OnSurfaceHeightChanged(double NewHeight) = 0;
};
