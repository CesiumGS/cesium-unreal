// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/MetadataArrayView.h"
#include "CesiumMetadataArray.h"
#include "CesiumMetadataValueType.h"
#include "CesiumMetadataGenericValue.generated.h"

/**
 * Provide a wrapper for scalar and array value.
 * This struct is provided to make it work with blueprint.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataGenericValue {
  GENERATED_USTRUCT_BODY()

private:
  using ValueType = std::variant<
      std::monostate,
      int8_t,
      uint8_t,
      int16_t,
      uint16_t,
      int32_t,
      uint32_t,
      int64_t,
      uint64_t,
      float,
      double,
      bool,
      std::string_view,
      CesiumGltf::MetadataArrayView<int8_t>,
      CesiumGltf::MetadataArrayView<uint8_t>,
      CesiumGltf::MetadataArrayView<int16_t>,
      CesiumGltf::MetadataArrayView<uint16_t>,
      CesiumGltf::MetadataArrayView<int32_t>,
      CesiumGltf::MetadataArrayView<uint32_t>,
      CesiumGltf::MetadataArrayView<int64_t>,
      CesiumGltf::MetadataArrayView<uint64_t>,
      CesiumGltf::MetadataArrayView<float>,
      CesiumGltf::MetadataArrayView<double>,
      CesiumGltf::MetadataArrayView<bool>,
      CesiumGltf::MetadataArrayView<std::string_view>>;

public:
  /**
   * Construct an empty value with unknown type.
   */
  FCesiumMetadataGenericValue()
      : _value(std::monostate{}), _type(ECesiumMetadataValueType::None) {}

  /**
   * Construct a scalar or array value.
   *
   * @param value The value to be stored in this struct
   */
  template <typename T>
  FCesiumMetadataGenericValue(const T& value)
      : _value(value), _type(ECesiumMetadataValueType::None) {
    if (std::holds_alternative<std::monostate>(_value)) {
      _type = ECesiumMetadataValueType::None;
    } else if (
        std::holds_alternative<int8_t>(_value) ||
        std::holds_alternative<uint8_t>(_value) ||
        std::holds_alternative<int16_t>(_value) ||
        std::holds_alternative<uint16_t>(_value) ||
        std::holds_alternative<int32_t>(_value) ||
        std::holds_alternative<uint32_t>(_value) ||
        std::holds_alternative<int64_t>(_value)) {
      _type = ECesiumMetadataValueType::Int64;
    } else if (std::holds_alternative<uint64_t>(_value)) {
      _type = ECesiumMetadataValueType::Uint64;
    } else if (std::holds_alternative<float>(_value)) {
      _type = ECesiumMetadataValueType::Float;
    } else if (std::holds_alternative<double>(_value)) {
      _type = ECesiumMetadataValueType::Double;
    } else if (std::holds_alternative<bool>(_value)) {
      _type = ECesiumMetadataValueType::Boolean;
    } else if (std::holds_alternative<std::string_view>(_value)) {
      _type = ECesiumMetadataValueType::String;
    } else {
      _type = ECesiumMetadataValueType::Array;
    }
  }

  /**
   * Query the type of the value.
   * This method should be used first before retrieving the stored value.
   * If the data that is tried to retrieve is different from the stored data
   * type, the current program will be aborted.
   *
   * @return The type of the value
   */
  ECesiumMetadataValueType GetType() const;

  /**
   * Retrieve the value as an int64_t.
   *
   * @return The int64_t value of the stored value
   */
  int64 GetInt64() const;

  /**
   * Retrieve the value as an uint64_t.
   *
   * @return The uint64_t value of the stored value
   */
  uint64 GetUint64() const;

  /**
   * Retrieve the value as a float.
   *
   * @return The float value of the stored value
   */
  float GetFloat() const;

  /**
   * Retrieve the value as a double.
   *
   * @return The double value of the stored value
   */
  double GetDouble() const;

  /**
   * Retrieve the value as a boolean.
   *
   * @return The boolean value of the stored value
   */
  bool GetBoolean() const;

  /**
   * Retrieve the value as a string.
   *
   * @return The string value of the stored value
   */
  FString GetString() const;

  /**
   * Retrieve the value as a generic array.
   *
   * @return The array value of the stored value
   */
  FCesiumMetadataArray GetArray() const;

  /**
   * Convert the stored value to string for display purpose.
   *
   * @return The converted string of the stored value.
   */
  FString ToString() const;

private:
  ValueType _value;
  ECesiumMetadataValueType _type;
};
