// Copyright 2020-2022 CesiumGS, Inc. and Contributors

#pragma once

#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION >= 5
#define CESIUM_UNREAL_ENGINE_DOUBLE 1
#else
#define CESIUM_UNREAL_ENGINE_DOUBLE 0
#endif

#if CESIUM_UNREAL_ENGINE_DOUBLE
using CesiumReal = double;
#else
using CesiumReal = float;
#endif
