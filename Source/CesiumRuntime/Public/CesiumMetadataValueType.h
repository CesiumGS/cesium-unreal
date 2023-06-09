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
  IntVector,
  FVector3f,
  FVector3,
  FVector4,
  Matrix,
  String,
  Array
};

// True types are cast, reintepreted, or parsed before being packed into gpu
// types when encoding into a texture.
enum class ECesiumMetadataPackedGpuType : uint8 { None, Uint8, Float };

// UE requires us to have an enum with the value 0.
// Invalid / None should have that value, but just make sure.
static_assert(int(CesiumGltf::PropertyType::Invalid) == 0);
static_assert(int(CesiumGltf::PropertyComponentType::None) == 0);

// TODO: deprecate
UENUM(BlueprintType)
enum class ECesiumMetadataTrueType : uint8 {
  None = 0,
  Int8,
  Uint8,
  Int16,
  Uint16,
  Int32,
  Uint32,
  Int64,
  Uint64,
  Float32,
  Float64,
  Boolean,
  Enum,
  String,
  Array
};

UENUM(BlueprintType)
enum class ECesiumMetadataType : uint8 {
  Invalid = 0,
  Scalar = int(CesiumGltf::PropertyType::Scalar),
  Vec2 = int(CesiumGltf::PropertyType::Vec2),
  Vec3 = int(CesiumGltf::PropertyType::Vec3),
  Vec4 = int(CesiumGltf::PropertyType::Vec4),
  Mat2 = int(CesiumGltf::PropertyType::Mat2),
  Mat3 = int(CesiumGltf::PropertyType::Mat3),
  Mat4 = int(CesiumGltf::PropertyType::Mat4),
  Boolean = int(CesiumGltf::PropertyType::Boolean),
  Enum = int(CesiumGltf::PropertyType::Enum),
  String = int(CesiumGltf::PropertyType::String),
};

UENUM(BlueprintType)
enum class ECesiumMetadataComponentType : uint8 {
  None = 0,
  Int8 = int(CesiumGltf::PropertyComponentType::Int8),
  Uint8 = int(CesiumGltf::PropertyComponentType::Uint8),
  Int16 = int(CesiumGltf::PropertyComponentType::Int16),
  Uint16 = int(CesiumGltf::PropertyComponentType::Uint16),
  Int32 = int(CesiumGltf::PropertyComponentType::Int32),
  Uint32 = int(CesiumGltf::PropertyComponentType::Uint32),
  Int64 = int(CesiumGltf::PropertyComponentType::Int64),
  Uint64 = int(CesiumGltf::PropertyComponentType::Uint64),
  Float32 = int(CesiumGltf::PropertyComponentType::Float32),
  Float64 = int(CesiumGltf::PropertyComponentType::Float64),
};

/**
 * Represents the true value type of a metadata property according to how the
 * property is defined in EXT_structural_metadata.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataValueType {
  GENERATED_USTRUCT_BODY()

  FCesiumMetadataValueType()
      : Type(ECesiumMetadataType::Invalid),
        ComponentType(ECesiumMetadataComponentType::None),
        bIsArray(false) {}

  FCesiumMetadataValueType(
      ECesiumMetadataType InType,
      ECesiumMetadataComponentType InComponentType,
      bool IsArray)
      : Type(InType), ComponentType(InComponentType), bIsArray(IsArray) {}

  ECesiumMetadataType Type;
  ECesiumMetadataComponentType ComponentType;
  bool bIsArray;
};
