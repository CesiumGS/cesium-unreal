// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataConversions.h"

ECesiumMetadataBlueprintType
CesiumMetadataValueTypeToBlueprintType(FCesiumMetadataValueType ValueType) {
  if (ValueType.bIsArray) {
    return ECesiumMetadataBlueprintType::Array;
  }

  ECesiumMetadataType type = ValueType.Type;
  ECesiumMetadataComponentType componentType = ValueType.ComponentType;

  if (type == ECesiumMetadataType::Boolean) {
    return ECesiumMetadataBlueprintType::Boolean;
  }

  if (type == ECesiumMetadataType::String) {
    return ECesiumMetadataBlueprintType::String;
  }

  if (type == ECesiumMetadataType::Scalar) {
    switch (componentType) {
    case ECesiumMetadataComponentType::Uint8:
      return ECesiumMetadataBlueprintType::Byte;
    case ECesiumMetadataComponentType::Int8:
    case ECesiumMetadataComponentType::Int16:
    case ECesiumMetadataComponentType::Uint16:
    case ECesiumMetadataComponentType::Int32:
      return ECesiumMetadataBlueprintType::Integer;
    case ECesiumMetadataComponentType::Uint32:
    case ECesiumMetadataComponentType::Int64:
      return ECesiumMetadataBlueprintType::Integer64;
    case ECesiumMetadataComponentType::Float32:
      return ECesiumMetadataBlueprintType::Float;
    case ECesiumMetadataComponentType::Float64:
      return ECesiumMetadataBlueprintType::Float64;
    case ECesiumMetadataComponentType::Uint64:
      return ECesiumMetadataBlueprintType::String;
    }
  }

  if (type == ECesiumMetadataType::Vec2) {
    return ECesiumMetadataBlueprintType::FVector2D;
  }

  if (type == ECesiumMetadataType::Vec3) {
    switch (componentType) {
    case ECesiumMetadataComponentType::Uint8:
    case ECesiumMetadataComponentType::Int8:
    case ECesiumMetadataComponentType::Int16:
    case ECesiumMetadataComponentType::Uint16:
    case ECesiumMetadataComponentType::Int32:
      return ECesiumMetadataBlueprintType::FIntVector;
    case ECesiumMetadataComponentType::Float32:
      return ECesiumMetadataBlueprintType::FVector3f;
    case ECesiumMetadataComponentType::Float64:
      return ECesiumMetadataBlueprintType::FVector3;
    }
  }

  if (type == ECesiumMetadataType::Vec4) {
    return ECesiumMetadataBlueprintType::FVector4;
  }

  if (type == ECesiumMetadataType::Mat4) {
    return ECesiumMetadataBlueprintType::FMatrix;
  }

  // Other matrix types can only be printed as strings.
  if (type == ECesiumMetadataType::Mat2 || type == ECesiumMetadataType::Mat3) {
    return ECesiumMetadataBlueprintType::String;
  }

  return ECesiumMetadataBlueprintType::None;
}

ECesiumMetadataPackedGpuType CesiumMetadataValueTypeToDefaultPackedGpuType(
    FCesiumMetadataValueType ValueType) {
  ECesiumMetadataType type = ValueType.Type;
  ECesiumMetadataComponentType componentType = ValueType.ComponentType;

  if (type == ECesiumMetadataType::Boolean) {
    return ECesiumMetadataPackedGpuType::Uint8;
  }

  if (type == ECesiumMetadataType::Scalar) {
    switch (componentType) {
    case ECesiumMetadataComponentType::Int8: // lossy or reinterpreted
    case ECesiumMetadataComponentType::Uint8:
      return ECesiumMetadataPackedGpuType::Uint8;
    case ECesiumMetadataComponentType::Float32:
    case ECesiumMetadataComponentType::Float64: // lossy
    case ECesiumMetadataComponentType::Int16:
    case ECesiumMetadataComponentType::Uint16:
    case ECesiumMetadataComponentType::Int32:  // lossy or reinterpreted
    case ECesiumMetadataComponentType::Uint32: // lossy or reinterpreted
    case ECesiumMetadataComponentType::Int64:  // lossy
    case ECesiumMetadataComponentType::Uint64: // lossy
      return ECesiumMetadataPackedGpuType::Float;
    default:
      return ECesiumMetadataPackedGpuType::None;
    }
  }

  return ECesiumMetadataPackedGpuType::None;
}
