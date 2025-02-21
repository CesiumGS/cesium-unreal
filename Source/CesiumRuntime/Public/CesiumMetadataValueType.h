// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/Enum.h"
#include "CesiumGltf/PropertyArrayView.h"
#include "CesiumGltf/PropertyType.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumMetadataEnum.h"
#include "CesiumMetadataValueType.generated.h"

/**
 * The Blueprint type that can losslessly represent values of a given property.
 */
UENUM(BlueprintType)
enum class ECesiumMetadataBlueprintType : uint8 {
  /* Indicates a value cannot be represented in Blueprints. */
  None,
  /* Indicates a value is best represented as a Boolean. */
  Boolean,
  /* Indicates a value is best represented as a Byte (8-bit unsigned integer).
   */
  Byte,
  /* Indicates a value is best represented as a Integer (32-bit signed). */
  Integer,
  /* Indicates a value is best represented as a Integer64 (64-bit signed). */
  Integer64,
  /* Indicates a value is best represented as a Float (32-bit). */
  Float,
  /* Indicates a value is best represented as a Float64 (64-bit). */
  Float64,
  /* Indicates a value is best represented as a FVector2D (2-dimensional
     integer vector). */
  IntPoint,
  /* Indicates a value is best represented as a FVector2D (2-dimensional
     double-precision vector). */
  Vector2D,
  /* Indicates a value is best represented as a FIntVector (3-dimensional
     integer vector). */
  IntVector,
  /* Indicates a value is best represented as a FVector3f (3-dimensional
     single-precision vector). */
  Vector3f,
  /* Indicates a value is best represented as a FVector3 (3-dimensional
     double-precision vector). */
  Vector3,
  /* Indicates a value is best represented as a FVector4 (4-dimensional
     double-precision vector). */
  Vector4,
  /* Indicates a value is best represented as a FMatrix (4-by-4 double-precision
     matrix). */
  Matrix,
  /* Indicates a value is best represented as a FString. This can be used as a
     fallback for types with no proper Blueprints representation. */
  String,
  /* Indicates a value is best represented as a CesiumPropertyArray. */
  Array
};

// UE requires us to have an enum with the value 0.
// Invalid / None should have that value, but just make sure.
static_assert(int(CesiumGltf::PropertyType::Invalid) == 0);
static_assert(int(CesiumGltf::PropertyComponentType::None) == 0);

/**
 * The type of a metadata property in EXT_feature_metadata. This has been
 * deprecated; use FCesiumMetadataValueType to get the complete type information
 * of a metadata property instead.
 */
UENUM(BlueprintType)
enum class ECesiumMetadataTrueType_DEPRECATED : uint8 {
  None_DEPRECATED = 0,
  Int8_DEPRECATED,
  Uint8_DEPRECATED,
  Int16_DEPRECATED,
  Uint16_DEPRECATED,
  Int32_DEPRECATED,
  Uint32_DEPRECATED,
  Int64_DEPRECATED,
  Uint64_DEPRECATED,
  Float32_DEPRECATED,
  Float64_DEPRECATED,
  Boolean_DEPRECATED,
  Enum_DEPRECATED,
  String_DEPRECATED,
  Array_DEPRECATED
};

// True types are cast, reintepreted, or parsed before being packed into gpu
// types when encoding into a texture.
enum class ECesiumMetadataPackedGpuType_DEPRECATED : uint8 {
  None_DEPRECATED,
  Uint8_DEPRECATED,
  Float_DEPRECATED
};

/**
 * The type of a metadata property in EXT_structural_metadata.
 */
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
  String = int(CesiumGltf::PropertyType::String),
  Enum = int(CesiumGltf::PropertyType::Enum)
};

template <typename T> struct TypeToCesiumMetadataType;

template <> struct TypeToCesiumMetadataType<uint8_t> {
  static constexpr ECesiumMetadataType value = ECesiumMetadataType::Scalar;
};
template <> struct TypeToCesiumMetadataType<int8_t> {
  static constexpr ECesiumMetadataType value = ECesiumMetadataType::Scalar;
};
template <> struct TypeToCesiumMetadataType<uint16_t> {
  static constexpr ECesiumMetadataType value = ECesiumMetadataType::Scalar;
};
template <> struct TypeToCesiumMetadataType<int16_t> {
  static constexpr ECesiumMetadataType value = ECesiumMetadataType::Scalar;
};
template <> struct TypeToCesiumMetadataType<uint32_t> {
  static constexpr ECesiumMetadataType value = ECesiumMetadataType::Scalar;
};
template <> struct TypeToCesiumMetadataType<int32_t> {
  static constexpr ECesiumMetadataType value = ECesiumMetadataType::Scalar;
};
template <> struct TypeToCesiumMetadataType<uint64_t> {
  static constexpr ECesiumMetadataType value = ECesiumMetadataType::Scalar;
};
template <> struct TypeToCesiumMetadataType<int64_t> {
  static constexpr ECesiumMetadataType value = ECesiumMetadataType::Scalar;
};
template <> struct TypeToCesiumMetadataType<float> {
  static constexpr ECesiumMetadataType value = ECesiumMetadataType::Scalar;
};
template <> struct TypeToCesiumMetadataType<double> {
  static constexpr ECesiumMetadataType value = ECesiumMetadataType::Scalar;
};
template <> struct TypeToCesiumMetadataType<std::string_view> {
  static constexpr ECesiumMetadataType value = ECesiumMetadataType::String;
};
template <> struct TypeToCesiumMetadataType<bool> {
  static constexpr ECesiumMetadataType value = ECesiumMetadataType::Boolean;
};
template <typename T, glm::qualifier P>
struct TypeToCesiumMetadataType<glm::vec<2, T, P>> {
  static constexpr ECesiumMetadataType value = ECesiumMetadataType::Vec2;
};
template <typename T, glm::qualifier P>
struct TypeToCesiumMetadataType<glm::vec<3, T, P>> {
  static constexpr ECesiumMetadataType value = ECesiumMetadataType::Vec3;
};
template <typename T, glm::qualifier P>
struct TypeToCesiumMetadataType<glm::vec<4, T, P>> {
  static constexpr ECesiumMetadataType value = ECesiumMetadataType::Vec4;
};
template <typename T, glm::qualifier P>
struct TypeToCesiumMetadataType<glm::mat<2, 2, T, P>> {
  static constexpr ECesiumMetadataType value = ECesiumMetadataType::Mat2;
};
template <typename T, glm::qualifier P>
struct TypeToCesiumMetadataType<glm::mat<3, 3, T, P>> {
  static constexpr ECesiumMetadataType value = ECesiumMetadataType::Mat3;
};
template <typename T, glm::qualifier P>
struct TypeToCesiumMetadataType<glm::mat<4, 4, T, P>> {
  static constexpr ECesiumMetadataType value = ECesiumMetadataType::Mat4;
};

/**
 * The component type of a metadata property in EXT_structural_metadata. Only
 * applicable if the property has a Scalar, VecN, or MatN type.
 */
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
 * Represents the true value type of a metadata value, akin to the property
 * types in EXT_structural_metadata.
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
      bool IsArray = false)
      : Type(InType), ComponentType(InComponentType), bIsArray(IsArray) {}

  /**
   * The type of the metadata property or value.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  ECesiumMetadataType Type;

  /**
   * The component of the metadata property or value. Only applies when the type
   * is a Scalar, VecN, or MatN type.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      Meta =
          (EditCondition =
               "Type != ECesiumMetadataType::Invalid && Type != ECesiumMetadataType::Boolean && Type != ECesiumMetadataType::Enum && Type != ECesiumMetadataType::String"))
  ECesiumMetadataComponentType ComponentType;

  /**
   * Whether or not this represents an array containing elements of the
   * specified types.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool bIsArray;

  inline bool operator==(const FCesiumMetadataValueType& ValueType) const {
    return Type == ValueType.Type && ComponentType == ValueType.ComponentType &&
           bIsArray == ValueType.bIsArray;
  }

  inline bool operator!=(const FCesiumMetadataValueType& ValueType) const {
    return Type != ValueType.Type || ComponentType != ValueType.ComponentType ||
           bIsArray != ValueType.bIsArray;
  }
};

template <typename T>
static FCesiumMetadataValueType
TypeToMetadataValueType(TSharedPtr<FCesiumMetadataEnum> pEnumDefinition) {
  ECesiumMetadataType type;
  ECesiumMetadataComponentType componentType;
  bool isArray;

  if constexpr (CesiumGltf::IsMetadataArray<T>::value) {
    using ArrayType = typename CesiumGltf::MetadataArrayType<T>::type;
    if (CesiumGltf::IsMetadataInteger<ArrayType>::value &&
        pEnumDefinition != nullptr) {
      type = ECesiumMetadataType::Enum;
    } else {
      type = TypeToCesiumMetadataType<ArrayType>::value;
    }
    componentType = ECesiumMetadataComponentType(
        CesiumGltf::TypeToPropertyComponentType<ArrayType>::component);
    isArray = true;
  } else {
    if (CesiumGltf::IsMetadataInteger<T>::value && pEnumDefinition.IsValid()) {
      type = ECesiumMetadataType::Enum;
      componentType = ECesiumMetadataComponentType(
          CesiumGltf::TypeToPropertyComponentType<T>::component);
    } else {
      type = TypeToCesiumMetadataType<T>::value;
      componentType = ECesiumMetadataComponentType(
          CesiumGltf::TypeToPropertyComponentType<T>::component);
    }
    isArray = false;
  }

  return {type, componentType, isArray};
}

/**
 * Gets the size in bytes of the represented metadata type. Returns 0 for enums
 * and strings.
 */
static size_t GetMetadataTypeByteSize(
    ECesiumMetadataType Type,
    ECesiumMetadataComponentType ComponentType) {
  size_t componentByteSize = 0;
  if (ComponentType != ECesiumMetadataComponentType::None)
    componentByteSize = CesiumGltf::getSizeOfComponentType(
        CesiumGltf::PropertyComponentType(ComponentType));

  size_t byteSize = componentByteSize;
  switch (Type) {
  case ECesiumMetadataType::Boolean:
    byteSize = sizeof(bool);
    break;
  case ECesiumMetadataType::Scalar:
    break;
  case ECesiumMetadataType::Vec2:
    byteSize *= 2;
    break;
  case ECesiumMetadataType::Vec3:
    byteSize *= 3;
    break;
  case ECesiumMetadataType::Vec4:
    byteSize *= 4;
    break;
  case ECesiumMetadataType::Mat2:
    byteSize *= 4;
    break;
  case ECesiumMetadataType::Mat3:
    byteSize *= 9;
    break;
  case ECesiumMetadataType::Mat4:
    byteSize *= 16;
    break;
  default:
    return 0;
  }

  return byteSize;
}
