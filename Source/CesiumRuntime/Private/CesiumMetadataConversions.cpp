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
    default:
      return ECesiumMetadataBlueprintType::String;
    }
  }

  if (type == ECesiumMetadataType::Vec2) {
    switch (componentType) {
    case ECesiumMetadataComponentType::Uint8:
    case ECesiumMetadataComponentType::Int8:
    case ECesiumMetadataComponentType::Int16:
    case ECesiumMetadataComponentType::Uint16:
    case ECesiumMetadataComponentType::Int32:
      return ECesiumMetadataBlueprintType::IntPoint;
    default:
      return ECesiumMetadataBlueprintType::Vector2D;
    }
  }

  if (type == ECesiumMetadataType::Vec3) {
    switch (componentType) {
    case ECesiumMetadataComponentType::Uint8:
    case ECesiumMetadataComponentType::Int8:
    case ECesiumMetadataComponentType::Int16:
    case ECesiumMetadataComponentType::Uint16:
    case ECesiumMetadataComponentType::Int32:
      return ECesiumMetadataBlueprintType::IntVector;
    case ECesiumMetadataComponentType::Float32:
      return ECesiumMetadataBlueprintType::Vector3f;
    default:
      return ECesiumMetadataBlueprintType::Vector3;
    }
  }

  if (type == ECesiumMetadataType::Vec4) {
    return ECesiumMetadataBlueprintType::Vector4;
  }

  if (type == ECesiumMetadataType::Mat2 || type == ECesiumMetadataType::Mat3 ||
      type == ECesiumMetadataType::Mat4) {
    return ECesiumMetadataBlueprintType::Matrix;
  }

  return ECesiumMetadataBlueprintType::None;
}

ECesiumMetadataBlueprintType CesiumMetadataTrueTypeToBlueprintType(
    ECesiumMetadataTrueType_DEPRECATED trueType) {
  switch (trueType) {
  case ECesiumMetadataTrueType_DEPRECATED::Boolean_DEPRECATED:
    return ECesiumMetadataBlueprintType::Boolean;
  case ECesiumMetadataTrueType_DEPRECATED::Uint8_DEPRECATED:
    return ECesiumMetadataBlueprintType::Byte;
  case ECesiumMetadataTrueType_DEPRECATED::Int8_DEPRECATED:
  case ECesiumMetadataTrueType_DEPRECATED::Int16_DEPRECATED:
  case ECesiumMetadataTrueType_DEPRECATED::Uint16_DEPRECATED:
  case ECesiumMetadataTrueType_DEPRECATED::Int32_DEPRECATED:
  // TODO: remove this one
  case ECesiumMetadataTrueType_DEPRECATED::Uint32_DEPRECATED:
    return ECesiumMetadataBlueprintType::Integer;
  case ECesiumMetadataTrueType_DEPRECATED::Int64_DEPRECATED:
    return ECesiumMetadataBlueprintType::Integer64;
  case ECesiumMetadataTrueType_DEPRECATED::Float32_DEPRECATED:
    return ECesiumMetadataBlueprintType::Float;
  case ECesiumMetadataTrueType_DEPRECATED::Float64_DEPRECATED:
    return ECesiumMetadataBlueprintType::Float64;
  case ECesiumMetadataTrueType_DEPRECATED::Uint64_DEPRECATED:
  case ECesiumMetadataTrueType_DEPRECATED::String_DEPRECATED:
    return ECesiumMetadataBlueprintType::String;
  case ECesiumMetadataTrueType_DEPRECATED::Array_DEPRECATED:
    return ECesiumMetadataBlueprintType::Array;
  default:
    return ECesiumMetadataBlueprintType::None;
  }
}

ECesiumMetadataTrueType_DEPRECATED
CesiumMetadataValueTypeToTrueType(FCesiumMetadataValueType ValueType) {
  if (ValueType.bIsArray) {
    return ECesiumMetadataTrueType_DEPRECATED::Array_DEPRECATED;
  }

  CesiumGltf::PropertyType type = CesiumGltf::PropertyType(ValueType.Type);
  CesiumGltf::PropertyComponentType componentType =
      CesiumGltf::PropertyComponentType(ValueType.ComponentType);

  if (type == CesiumGltf::PropertyType::Boolean) {
    return ECesiumMetadataTrueType_DEPRECATED::Boolean_DEPRECATED;
  }

  if (type == CesiumGltf::PropertyType::Scalar) {
    switch (componentType) {
    case CesiumGltf::PropertyComponentType::Uint8:
      return ECesiumMetadataTrueType_DEPRECATED::Uint8_DEPRECATED;
    case CesiumGltf::PropertyComponentType::Int8:
      return ECesiumMetadataTrueType_DEPRECATED::Int8_DEPRECATED;
    case CesiumGltf::PropertyComponentType::Uint16:
      return ECesiumMetadataTrueType_DEPRECATED::Uint16_DEPRECATED;
    case CesiumGltf::PropertyComponentType::Int16:
      return ECesiumMetadataTrueType_DEPRECATED::Int16_DEPRECATED;
    case CesiumGltf::PropertyComponentType::Uint32:
      return ECesiumMetadataTrueType_DEPRECATED::Uint32_DEPRECATED;
    case CesiumGltf::PropertyComponentType::Int32:
      return ECesiumMetadataTrueType_DEPRECATED::Int32_DEPRECATED;
    case CesiumGltf::PropertyComponentType::Int64:
      return ECesiumMetadataTrueType_DEPRECATED::Int64_DEPRECATED;
    case CesiumGltf::PropertyComponentType::Uint64:
      return ECesiumMetadataTrueType_DEPRECATED::Uint64_DEPRECATED;
    case CesiumGltf::PropertyComponentType::Float32:
      return ECesiumMetadataTrueType_DEPRECATED::Float32_DEPRECATED;
    case CesiumGltf::PropertyComponentType::Float64:
      return ECesiumMetadataTrueType_DEPRECATED::Float64_DEPRECATED;
    default:
      return ECesiumMetadataTrueType_DEPRECATED::None_DEPRECATED;
    }
  }

  if (type == CesiumGltf::PropertyType::String) {
    return ECesiumMetadataTrueType_DEPRECATED::String_DEPRECATED;
  }

  return ECesiumMetadataTrueType_DEPRECATED::None_DEPRECATED;
}
