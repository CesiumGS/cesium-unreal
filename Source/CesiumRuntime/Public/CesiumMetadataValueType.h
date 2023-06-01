// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/PropertyType.h"
#include "CesiumGltf/StructuralMetadataPropertyType.h"
#include "UObject/ObjectMacros.h"
#include "CesiumMetadataValueType.generated.h"

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
  Float64,
  FVector,
  FVector4,
  // TODO: find all vector and matrix types
  String,
  Array
};

// True types are cast, reintepreted, or parsed before being packed into gpu
// types when encoding into a texture.
enum class ECesiumMetadataPackedGpuType : uint8 { None, Uint8, Float };

// UE requires us to have an enum with the value 0.
// Invalid / None should have that value, but just make sure.
static_assert(int(CesiumGltf::StructuralMetadata::PropertyType::Invalid) == 0);
static_assert(
    int(CesiumGltf::StructuralMetadata::PropertyComponentType::None) == 0);

// TODO: deprecate
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

UENUM(BlueprintType)
enum class ECesiumMetadataType : uint8 {
  Invalid = 0,
  Scalar = int(CesiumGltf::StructuralMetadata::PropertyType::Scalar),
  Vec2 = int(CesiumGltf::StructuralMetadata::PropertyType::Vec2),
  Vec3 = int(CesiumGltf::StructuralMetadata::PropertyType::Vec3),
  Vec4 = int(CesiumGltf::StructuralMetadata::PropertyType::Vec4),
  Mat2 = int(CesiumGltf::StructuralMetadata::PropertyType::Mat2),
  Mat3 = int(CesiumGltf::StructuralMetadata::PropertyType::Mat3),
  Mat4 = int(CesiumGltf::StructuralMetadata::PropertyType::Mat4),
  Boolean = int(CesiumGltf::StructuralMetadata::PropertyType::Boolean),
  Enum = int(CesiumGltf::StructuralMetadata::PropertyType::Enum),
  String = int(CesiumGltf::StructuralMetadata::PropertyType::String),
};

UENUM(BlueprintType)
enum class ECesiumMetadataComponentType : uint8 {
  None = 0,
  Int8 = int(CesiumGltf::StructuralMetadata::PropertyComponentType::Int8),
  Uint8 = int(CesiumGltf::StructuralMetadata::PropertyComponentType::Uint8),
  Int16 = int(CesiumGltf::StructuralMetadata::PropertyComponentType::Int16),
  Uint16 = int(CesiumGltf::StructuralMetadata::PropertyComponentType::Uint16),
  Int32 = int(CesiumGltf::StructuralMetadata::PropertyComponentType::Int32),
  Uint32 = int(CesiumGltf::StructuralMetadata::PropertyComponentType::Uint32),
  Int64 = int(CesiumGltf::StructuralMetadata::PropertyComponentType::Int64),
  Uint64 = int(CesiumGltf::StructuralMetadata::PropertyComponentType::Uint64),
  Float32 = int(CesiumGltf::StructuralMetadata::PropertyComponentType::Float32),
  Float64 = int(CesiumGltf::StructuralMetadata::PropertyComponentType::Float64),
};

/**
 * Represents the true types of a metadata property according to how the
 * property is defined in the metadata extension.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataTypes {
  GENERATED_USTRUCT_BODY()
  ECesiumMetadataType Type;
  ECesiumMetadataComponentType ComponentType;
  bool bIsArray;
};
