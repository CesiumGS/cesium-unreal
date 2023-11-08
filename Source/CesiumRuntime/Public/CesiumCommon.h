// Copyright 2020-2022 CesiumGS, Inc. and Contributors

#pragma once

#include "Runtime/Launch/Resources/Version.h"

#define ENGINE_VERSION_5_3_OR_HIGHER                                           \
  (ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3)
#define ENGINE_VERSION_5_2_OR_HIGHER                                           \
  (ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2)
