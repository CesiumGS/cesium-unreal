// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "UnrealMetadataConversions.h"

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

/*static*/ FIntPoint
UnrealMetadataConversions::toIntPoint(const glm::ivec2& vec2) {
  return FIntPoint(vec2[0], vec2[1]);
}

/*static*/ FIntPoint UnrealMetadataConversions::toIntPoint(
    const std::string_view& string,
    const FIntPoint& defaultValue) {
  FString unrealString = UnrealMetadataConversions::toString(string);

  // For some reason, FIntPoint doesn't have an InitFromString method, so
  // copy the one from FVector.
  int32 X = 0, Y = 0;
  const bool bSuccessful = FParse::Value(*unrealString, TEXT("X="), X) &&
                           FParse::Value(*unrealString, TEXT("Y="), Y);
  return bSuccessful ? FIntPoint(X, Y) : defaultValue;
}

/*static*/ FVector2D
UnrealMetadataConversions::toVector2D(const glm::dvec2& vec2) {
  return FVector2D(vec2[0], vec2[1]);
}

/*static*/ FVector2D UnrealMetadataConversions::toVector2D(
    const std::string_view& string,
    const FVector2D& defaultValue) {
  FString unrealString = UnrealMetadataConversions::toString(string);
  FVector2D result;
  return result.InitFromString(unrealString) ? result : defaultValue;
}

/*static*/ FIntVector
UnrealMetadataConversions::toIntVector(const glm::ivec3& vec3) {
  return FIntVector(vec3[0], vec3[1], vec3[2]);
}

/*static*/ FIntVector UnrealMetadataConversions::toIntVector(
    const std::string_view& string,
    const FIntVector& defaultValue) {
  FString unrealString = UnrealMetadataConversions::toString(string);
  // For some reason, FIntVector doesn't have an InitFromString method, so
  // copy the one from FVector.
  int32 X = 0, Y = 0, Z = 0;
  const bool bSuccessful = FParse::Value(*unrealString, TEXT("X="), X) &&
                           FParse::Value(*unrealString, TEXT("Y="), Y) &&
                           FParse::Value(*unrealString, TEXT("Z="), Z);
  return bSuccessful ? FIntVector(X, Y, Z) : defaultValue;
}

/*static*/ FVector3f
UnrealMetadataConversions::toVector3f(const glm::vec3& vec3) {
  return FVector3f(vec3[0], vec3[1], vec3[2]);
}

/*static*/ FVector3f UnrealMetadataConversions::toVector3f(
    const std::string_view& string,
    const FVector3f& defaultValue) {
  FString unrealString = UnrealMetadataConversions::toString(string);
  FVector3f result;
  return result.InitFromString(unrealString) ? result : defaultValue;
}

/*static*/ FVector UnrealMetadataConversions::toVector(const glm::dvec3& vec3) {
  return FVector(vec3[0], vec3[1], vec3[2]);
}

/*static*/ FVector UnrealMetadataConversions::toVector(
    const std::string_view& string,
    const FVector& defaultValue) {
  FString unrealString = UnrealMetadataConversions::toString(string);
  FVector result;
  return result.InitFromString(unrealString) ? result : defaultValue;
}

/*static*/ FVector4
UnrealMetadataConversions::toVector4(const glm::dvec4& vec4) {
  return FVector4(vec4[0], vec4[1], vec4[2], vec4[3]);
}

/*static*/ FVector4 UnrealMetadataConversions::toVector4(
    const std::string_view& string,
    const FVector4& defaultValue) {
  FString unrealString = UnrealMetadataConversions::toString(string);
  FVector4 result;
  return result.InitFromString(unrealString) ? result : defaultValue;
}

/*static*/ FMatrix UnrealMetadataConversions::toMatrix(const glm::dmat4& mat4) {
  // glm is column major, but Unreal is row major.
  FPlane4d row1(
      static_cast<double>(mat4[0][0]),
      static_cast<double>(mat4[1][0]),
      static_cast<double>(mat4[2][0]),
      static_cast<double>(mat4[3][0]));

  FPlane4d row2(
      static_cast<double>(mat4[0][1]),
      static_cast<double>(mat4[1][1]),
      static_cast<double>(mat4[2][1]),
      static_cast<double>(mat4[3][1]));

  FPlane4d row3(
      static_cast<double>(mat4[0][2]),
      static_cast<double>(mat4[1][2]),
      static_cast<double>(mat4[2][2]),
      static_cast<double>(mat4[3][2]));

  FPlane4d row4(
      static_cast<double>(mat4[0][3]),
      static_cast<double>(mat4[1][3]),
      static_cast<double>(mat4[2][3]),
      static_cast<double>(mat4[3][3]));

  return FMatrix(row1, row2, row3, row4);
}

/*static*/ FString
UnrealMetadataConversions::toString(const std::string_view& from) {
  return toString(std::string(from));
}

/*static*/ FString
UnrealMetadataConversions::toString(const std::string& from) {
  return FString(UTF8_TO_TCHAR(from.data()));
}
