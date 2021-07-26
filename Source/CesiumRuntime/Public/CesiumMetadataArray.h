// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/MetadataArrayView.h"
#include "CesiumMetadataValueType.h"
#include <string_view>
#include <variant>
#include "CesiumMetadataArray.generated.h"

/**
 * Provide a wrapper for the view of metadata array value.
 * This struct is provided to make it work with blueprint.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataArray {
  GENERATED_USTRUCT_BODY()

private:
  using ArrayType = std::variant<
      std::monostate,
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
   * Construct an empty array with unknown type.
   */
  FCesiumMetadataArray()
      : _value(std::monostate{}), _type(ECesiumMetadataValueType::None) {}

  /**
   * Construct a non-empty array.
   * @param value The metadata array view that will be stored in this struct
   */
  template <typename T>
  FCesiumMetadataArray(CesiumGltf::MetadataArrayView<T> value)
      : _value(value), _type(ECesiumMetadataValueType::None) {
    if (holdsArrayAlternative<int8_t>(_value) ||
        holdsArrayAlternative<uint8_t>(_value) ||
        holdsArrayAlternative<int16_t>(_value) ||
        holdsArrayAlternative<uint16_t>(_value) ||
        holdsArrayAlternative<int32_t>(_value) ||
        holdsArrayAlternative<uint32_t>(_value) ||
        holdsArrayAlternative<int64_t>(_value)) {
      _type = ECesiumMetadataValueType::Int64;
    } else if (holdsArrayAlternative<uint64_t>(_value)) {
      _type = ECesiumMetadataValueType::Uint64;
    } else if (holdsArrayAlternative<float>(_value)) {
      _type = ECesiumMetadataValueType::Float;
    } else if (holdsArrayAlternative<double>(_value)) {
      _type = ECesiumMetadataValueType::Double;
    } else if (holdsArrayAlternative<bool>(_value)) {
      _type = ECesiumMetadataValueType::Boolean;
    } else {
      _type = ECesiumMetadataValueType::String;
    }
  }

  /**
   * Query the component type of the array value.
   * This method should be used first before retrieving the data of the array.
   * If the data that is tried to retrieve is different from the stored data
   * type, the current program will be aborted.
   *
   * @return The component type of the array
   */
  ECesiumMetadataValueType GetComponentType() const;

  /**
   * Query the number of elements in the array.
   * This method returns 0 if component type is ECesiumMetadataValueType::None
   *
   * @return The number of elements in the array
   */
  size_t GetSize() const;

  /**
   * Retrieve the component at index i as an int64_t value.
   *
   * @param i The index of the component
   * @return The int64_t value of the component
   */
  int64 GetInt64(size_t i) const;

  /**
   * Retrieve the component at index i as an uint64_t value.
   *
   * @param i The index of the component
   * @return The uint64_t value of the component
   */
  uint64 GetUint64(size_t i) const;

  /**
   * Retrieve the component at index i as a float value.
   *
   * @param i The index of the component
   * @return The float value of the component
   */
  float GetFloat(size_t i) const;

  /**
   * Retrieve the component at index i as a double value.
   *
   * @param i The index of the component
   * @return The double value of the component
   */
  double GetDouble(size_t i) const;

  /**
   * Retrieve the component at index i as a boolean value.
   *
   * @param i The index of the component
   * @return The boolean value of the component
   */
  bool GetBoolean(size_t i) const;

  /**
   * Retrieve the component at index i as a string value.
   *
   * @param i The index of the component
   * @return The string value of the component
   */
  FString GetString(size_t i) const;

private:
  template <typename T, typename... VariantType>
  static bool
  holdsArrayAlternative(const std::variant<VariantType...>& variant) {
    return std::holds_alternative<CesiumGltf::MetadataArrayView<T>>(variant);
  }

  ArrayType _value;
  ECesiumMetadataValueType _type;
};
