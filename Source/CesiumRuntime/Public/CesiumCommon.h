// Copyright 2020-2022 CesiumGS, Inc. and Contributors

#pragma once

#include "Runtime/Launch/Resources/Version.h"

#define CESIUM_UNREAL_ENGINE_DOUBLE 1

#if CESIUM_UNREAL_ENGINE_DOUBLE
using CesiumReal = double;
#else
using CesiumReal = float;
#endif
