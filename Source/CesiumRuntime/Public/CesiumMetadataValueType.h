// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Determine the type of the metadata value.
 * The enum should be used first before retrieving the
 * stored value in FCesiumMetadataArray or FCesiumMetadataGenericValue.
 * If the stored value type is different with what the enum reports, it will
 * abort the program.
 */
UENUM(BlueprintType)
enum class ECesiumMetadataValueType : uint8 {
  Int64,
  Uint64,
  Float,
  Double,
  Boolean,
  String,
  Array,
  None
};
