// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/MetadataPropertyView.h"
#include "CesiumMetadataArray.h"
#include "CesiumMetadataGenericValue.h"
#include "CesiumMetadataValueType.h"
#include "UObject/ObjectMacros.h"
#include <string_view>
#include <variant>
#include "CesiumMetadataProperty.generated.h"

/**
 * Provide a wrapper for a metadata property. Each value in
 * the property represents the value of a feature's metadata value
 * at that property field in the feature table.
 * This struct is provided to make it work with blueprint.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataProperty {
  GENERATED_USTRUCT_BODY()

private:
  using PropertyType = std::variant<
      std::monostate,
      CesiumGltf::MetadataPropertyView<int8_t>,
      CesiumGltf::MetadataPropertyView<uint8_t>,
      CesiumGltf::MetadataPropertyView<int16_t>,
      CesiumGltf::MetadataPropertyView<uint16_t>,
      CesiumGltf::MetadataPropertyView<int32_t>,
      CesiumGltf::MetadataPropertyView<uint32_t>,
      CesiumGltf::MetadataPropertyView<int64_t>,
      CesiumGltf::MetadataPropertyView<uint64_t>,
      CesiumGltf::MetadataPropertyView<float>,
      CesiumGltf::MetadataPropertyView<double>,
      CesiumGltf::MetadataPropertyView<bool>,
      CesiumGltf::MetadataPropertyView<std::string_view>,
      CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<int8_t>>,
      CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<uint8_t>>,
      CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<int16_t>>,
      CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<uint16_t>>,
      CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<int32_t>>,
      CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<uint32_t>>,
      CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<int64_t>>,
      CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<uint64_t>>,
      CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<float>>,
      CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<double>>,
      CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<bool>>,
      CesiumGltf::MetadataPropertyView<
          CesiumGltf::MetadataArrayView<std::string_view>>>;

public:
  /**
   * Construct an empty property with unknown type.
   */
  FCesiumMetadataProperty()
      : _property(std::monostate{}), _type(ECesiumMetadataValueType::None) {}

  /**
   * Construct a wrapper for the property view.
   *
   * @param value The property to be stored in this struct
   */
  template <typename T>
  FCesiumMetadataProperty(const CesiumGltf::MetadataPropertyView<T>& value)
      : _property(value), _type(ECesiumMetadataValueType::None) {
    if (holdsPropertyAlternative<int8_t>(_property) ||
        holdsPropertyAlternative<uint8_t>(_property) ||
        holdsPropertyAlternative<int16_t>(_property) ||
        holdsPropertyAlternative<uint16_t>(_property) ||
        holdsPropertyAlternative<int32_t>(_property) ||
        holdsPropertyAlternative<uint32_t>(_property) ||
        holdsPropertyAlternative<int64_t>(_property)) {
      _type = ECesiumMetadataValueType::Int64;
    } else if (holdsPropertyAlternative<uint64_t>(_property)) {
      _type = ECesiumMetadataValueType::Uint64;
    } else if (holdsPropertyAlternative<float>(_property)) {
      _type = ECesiumMetadataValueType::Float;
    } else if (holdsPropertyAlternative<double>(_property)) {
      _type = ECesiumMetadataValueType::Double;
    } else if (holdsPropertyAlternative<bool>(_property)) {
      _type = ECesiumMetadataValueType::Boolean;
    } else if (holdsPropertyAlternative<std::string_view>(_property)) {
      _type = ECesiumMetadataValueType::String;
    } else {
      _type = ECesiumMetadataValueType::Array;
    }
  }

  /**
   * Query the type of the property.
   * This method should be used first before retrieving the stored value.
   * If the data that is tried to retrieve is different from the stored data
   * type, the current program will be aborted.
   *
   * @return The type of the value
   */
  ECesiumMetadataValueType GetType() const;

  /**
   * Query the number of features in the property.
   *
   * @return The number of features in the property
   */
  size_t GetNumOfFeatures() const;

  /**
   * Retrieve the feature value at index featureID as a boolean value.
   *
   * @param featureID The index of the feature
   * @return The boolean value at the featureID
   */
  bool GetBoolean(size_t featureID) const;

  /**
   * Retrieve the feature value at index featureID as an int64_t value.
   *
   * @param featureID The index of the feature
   * @return The int64_t value at the featureID
   */
  int64 GetInt64(size_t featureID) const;

  /**
   * Retrieve the feature value at index featureID as an uint64_t value.
   *
   * @param featureID The index of the feature
   * @return The uint64_t value at the featureID
   */
  uint64 GetUint64(size_t featureID) const;

  /**
   * Retrieve the feature value at index featureID as a float value.
   *
   * @param featureID The index of the feature
   * @return The float value at the featureID
   */
  float GetFloat(size_t featureID) const;

  /**
   * Retrieve the feature value at index featureID as a double value.
   *
   * @param featureID The index of the feature
   * @return The double value at the featureID
   */
  double GetDouble(size_t featureID) const;

  /**
   * Retrieve the feature value at index featureID as a string value.
   *
   * @param featureID The index of the feature
   * @return The string value at the featureID
   */
  FString GetString(size_t featureID) const;

  /**
   * Retrieve the feature value at index featureID as an array value.
   *
   * @param featureID The index of the feature
   * @return The array value at the featureID
   */
  FCesiumMetadataArray GetArray(size_t featureID) const;

  /**
   * Convert the underlying value to a generic value. Convenient for storing
   * the value in the container like TArray or TMap
   *
   * @param featureID The index of the feature
   * @return The generic value at the featureID
   */
  FCesiumMetadataGenericValue GetGenericValue(size_t featureID) const;

private:
  template <typename T, typename... VariantType>
  static bool
  holdsPropertyAlternative(const std::variant<VariantType...>& variant) {
    return std::holds_alternative<CesiumGltf::MetadataPropertyView<T>>(variant);
  }

  PropertyType _property;
  ECesiumMetadataValueType _type;
};
