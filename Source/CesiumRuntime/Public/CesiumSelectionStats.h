// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Delegates/IDelegateInstance.h"

/**
 * @brief Displays information about Cesium 3D Tiles selected in the Editor.
 */
class CESIUMRUNTIME_API CesiumSelectionStats {
public:
  CesiumSelectionStats() noexcept;
  ~CesiumSelectionStats() noexcept;

private:
  FDelegateHandle _selectionChangedHandle;
};
