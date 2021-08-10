// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/PropertyType.h"
#include "UObject/ObjectMacros.h"

/**
 * The Blueprint type that can losslessly represent values of a given property.
 */
UENUM(BlueprintType)
enum class ECesiumMetadataBlueprintType : uint8 {
  None,
  Boolean,
  Byte,
  Integer,
  Integer64,
  Float,
  String,
  Array
};

// UE requires us to have an enum with the value 0.
// None should have that value, but make sure.
static_assert(int(CesiumGltf::PropertyType::None) == 0);

UENUM(BlueprintType)
enum class ECesiumMetadataTrueType : uint8 {
  None = 0,
  Int8 = int(CesiumGltf::PropertyType::Int8),
  Uint8 = int(CesiumGltf::PropertyType::Uint8),
  Int16 = int(CesiumGltf::PropertyType::Int16),
  Uint16 = int(CesiumGltf::PropertyType::Uint16),
  Int32 = int(CesiumGltf::PropertyType::Int32),
  Uint32 = int(CesiumGltf::PropertyType::Uint32),
  Int64 = int(CesiumGltf::PropertyType::Int64),
  Uint64 = int(CesiumGltf::PropertyType::Uint64),
  Float32 = int(CesiumGltf::PropertyType::Float32),
  Float64 = int(CesiumGltf::PropertyType::Float64),
  Boolean = int(CesiumGltf::PropertyType::Boolean),
  Enum = int(CesiumGltf::PropertyType::Enum),
  String = int(CesiumGltf::PropertyType::String),
  Array = int(CesiumGltf::PropertyType::Array)
};
