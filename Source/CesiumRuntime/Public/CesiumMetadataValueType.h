// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "UObject/ObjectMacros.h"

/**
 * The Blueprint type that can losslessly represent values of a given property.
 */
UENUM(BlueprintType)
enum class ECesiumMetadataValueType : uint8 {
  None,
  Boolean,
  Byte,
  Integer,
  Integer64,
  Float,
  String,
  Array
};
