// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataConversions.h"

ECesiumMetadataBlueprintType
CesiuMetadataTrueTypeToBlueprintType(ECesiumMetadataTrueType trueType) {
  switch (trueType) {
  case ECesiumMetadataTrueType::Boolean:
    return ECesiumMetadataBlueprintType::Boolean;
  case ECesiumMetadataTrueType::Uint8:
    return ECesiumMetadataBlueprintType::Byte;
  case ECesiumMetadataTrueType::Int8:
  case ECesiumMetadataTrueType::Int16:
  case ECesiumMetadataTrueType::Uint16:
  case ECesiumMetadataTrueType::Int32:
  // TODO: remove this one
  case ECesiumMetadataTrueType::Uint32:
    return ECesiumMetadataBlueprintType::Integer;
  case ECesiumMetadataTrueType::Int64:
    return ECesiumMetadataBlueprintType::Integer64;
  case ECesiumMetadataTrueType::Float32:
    return ECesiumMetadataBlueprintType::Float;
  case ECesiumMetadataTrueType::Float64:
    return ECesiumMetadataBlueprintType::Float64;
  case ECesiumMetadataTrueType::Uint64:
  case ECesiumMetadataTrueType::String:
    return ECesiumMetadataBlueprintType::String;
  case ECesiumMetadataTrueType::Array:
    return ECesiumMetadataBlueprintType::Array;
  default:
    return ECesiumMetadataBlueprintType::None;
  }
}

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
    case ECesiumMetadataComponentType::Uint32:
      return ECesiumMetadataBlueprintType::Integer;
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

  if (type == ECesiumMetadataType::Vec3) {
    switch (componentType) {
    case ECesiumMetadataComponentType::Float32:
      return ECesiumMetadataBlueprintType::FVector3f;
    case ECesiumMetadataComponentType::Float64:
      return ECesiumMetadataBlueprintType::FVector3;
    }
  }

  // TODO: vec and mat

  return ECesiumMetadataBlueprintType::None;
}

ECesiumMetadataPackedGpuType
CesiumMetadataTrueTypeToDefaultPackedGpuType(ECesiumMetadataTrueType trueType) {
  switch (trueType) {
  case ECesiumMetadataTrueType::Boolean:
  case ECesiumMetadataTrueType::Int8: // lossy or reinterpreted
  case ECesiumMetadataTrueType::Uint8:
    return ECesiumMetadataPackedGpuType::Uint8;
  case ECesiumMetadataTrueType::Float32:
  case ECesiumMetadataTrueType::Float64: // lossy
  case ECesiumMetadataTrueType::Int16:
  case ECesiumMetadataTrueType::Uint16:
  case ECesiumMetadataTrueType::Int32:  // lossy or reinterpreted
  case ECesiumMetadataTrueType::Uint32: // lossy or reinterpreted
  case ECesiumMetadataTrueType::Int64:  // lossy
  case ECesiumMetadataTrueType::Uint64: // lossy
    return ECesiumMetadataPackedGpuType::Float;
  default:
    return ECesiumMetadataPackedGpuType::None;
  }
}

ECesiumMetadataPackedGpuType
CesiumMetadataTypesToDefaultPackedGpuType(FCesiumMetadataValueType ValueType) {
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
