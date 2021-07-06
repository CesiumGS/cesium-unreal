#pragma once

#include "CesiumGltf/AccessorView.h"
#include "CesiumGltf/FeatureIDAttribute.h"
#include "CesiumGltf/MeshPrimitiveEXT_feature_metadata.h"
#include "CesiumGltf/MetadataArrayView.h"
#include "CesiumGltf/MetadataPropertyView.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/ModelEXT_feature_metadata.h"
#include "CoreMinimal.h"
#include <variant>
#include "CesiumMetadata.generated.h"

/**
 * Determine the type of the metadata value.
 * The enum should be used first before retrieving the
 * stored value in FCesiumMetadataArray or FCesiumMetadataGenericValue.
 * If the stored value type is different with what the enum reports, it will
 * abort the program.
 */
UENUM(BlueprintType)
enum class ECesiumMetadataValueType : uint8 {
  Int64,
  Uint64,
  Float,
  Double,
  Boolean,
  String,
  Array,
  None
};

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

/**
 * Provide a wrapper for a metadata feature table. Feature table is a
 * collection of properties and stores the value as a struct of array.
 * This struct is provided to make it work with blueprint.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataFeatureTable {
  GENERATED_USTRUCT_BODY()

  using AccesorViewType = std::variant<
      std::monostate,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<int8_t>>,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint8_t>>,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<int16_t>>,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint16_t>>,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint32_t>>,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<float>>>;

public:
  /**
   * Construct an empty feature table.
   */
  FCesiumMetadataFeatureTable(){};

  /**
   * Construct a feature table from a Gltf Feature Table.
   *
   * @param model The model that stores EXT_feature_metadata
   * @param accessor The accessor of feature ID
   * @param featureTable The feature table that is paired with feature ID
   */
  FCesiumMetadataFeatureTable(
      const CesiumGltf::Model& model,
      const CesiumGltf::Accessor& accessor,
      const CesiumGltf::FeatureTable& featureTable);

  /**
   * Query the number of features in the feature table.
   *
   * @return The number of features in the feature table
   */
  size_t GetNumOfFeatures() const;

  /**
   * Query the feature ID based on a vertex.
   *
   * @return The feature ID of the vertex
   */
  int64 GetFeatureIDForVertex(uint32 vertexIdx) const;

  /**
   * Return the map of a feature that maps feature's property name to value.
   *
   * @return The property map of a feature
   */
  TMap<FString, FCesiumMetadataGenericValue>
  GetValuesForFeatureID(size_t featureID) const;

  /**
   * Return the map of a feature that maps feature's property name to value as
   * string.
   *
   * @return The property map of a feature
   */
  TMap<FString, FString> GetValuesAsStringsForFeatureID(size_t featureID) const;

  /**
   * Get all the properties of a feature table.
   *
   * @return All the properties of a feature table
   */
  const TMap<FString, FCesiumMetadataProperty>& GetProperties() const;

private:
  AccesorViewType _featureIDAccessor;
  TMap<FString, FCesiumMetadataProperty> _properties;
};

/**
 * Provide a wrapper for a Gltf Primitive's Metadata. Each primitive
 * metadata is a collection of feature tables
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataPrimitive {
  GENERATED_USTRUCT_BODY()

public:
  /**
   * Construct an empty primitive metadata.
   */
  FCesiumMetadataPrimitive() {}

  /**
   * Construct a primitive metadata.
   *
   * @param model The model that stores EXT_feature_metadata extension
   * @param primitive The mesh primitive that stores EXT_feature_metadata
   * extension
   * @param metadata The EXT_feature_metadata of the whole gltf
   * @param primitiveMetadata The EXT_feature_metadata of the gltf mesh
   * primitive
   */
  FCesiumMetadataPrimitive(
      const CesiumGltf::Model& model,
      const CesiumGltf::MeshPrimitive& primitive,
      const CesiumGltf::ModelEXT_feature_metadata& metadata,
      const CesiumGltf::MeshPrimitiveEXT_feature_metadata& primitiveMetadata);

  /**
   * Get all the feature tables that are associated with the primitive.
   *
   * @return All the feature tables that are associated with the primitive
   */
  const TArray<FCesiumMetadataFeatureTable>& GetFeatureTables() const;

private:
  TArray<FCesiumMetadataFeatureTable> _featureTables;
};
