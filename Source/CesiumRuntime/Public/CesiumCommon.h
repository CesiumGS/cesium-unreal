// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Runtime/Launch/Resources/Version.h"

#define ENGINE_VERSION_5_5_OR_HIGHER                                           \
  (ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 5)
#define ENGINE_VERSION_5_4_OR_HIGHER                                           \
  (ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 4)
#define ENGINE_VERSION_5_3_OR_HIGHER                                           \
  (ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3)
