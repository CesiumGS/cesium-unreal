#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/ModelEXT_feature_metadata.h"
#include "CesiumGltf/MetadataArrayView.h"
#include "CesiumGltf/MetadataPropertyView.h"
#include "CesiumGltf/FeatureIDAttribute.h"
#include <variant>
#include "CesiumMetadata.generated.h"

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
  FCesiumMetadataArray() 
    : _value(std::monostate{}), _type(ECesiumMetadataValueType::None)
  {}

  template<typename T>
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
    }

    if (holdsArrayAlternative<uint64_t>(_value)) {
      _type = ECesiumMetadataValueType::Uint64;
    }

    if (holdsArrayAlternative<float>(_value)) {
      _type = ECesiumMetadataValueType::Float;
    }

    if (holdsArrayAlternative<double>(_value)) {
      _type = ECesiumMetadataValueType::Double;
    }

    if (holdsArrayAlternative<bool>(_value)) {
      _type = ECesiumMetadataValueType::Boolean;
    }

    _type = ECesiumMetadataValueType::String;
  }

  ECesiumMetadataValueType GetComponentType() const;

  size_t GetSize() const;

  int64 GetInt64(size_t i) const;

  uint64 GetUint64(size_t i) const;

  float GetFloat(size_t i) const;

  double GetDouble(size_t i) const;

  bool GetBoolean(size_t i) const;

  FString GetString(size_t i) const;

private:
  template<typename T, typename ...VariantType>
  static bool holdsArrayAlternative(const std::variant<VariantType...>& variant) {
    return std::holds_alternative<CesiumGltf::MetadataArrayView<T>>(variant);
  }

  ArrayType _value;
  ECesiumMetadataValueType _type;
};

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
  FCesiumMetadataGenericValue() 
    : _value(std::monostate{}), _type(ECesiumMetadataValueType::None)
  {}

  template<typename T>
  FCesiumMetadataGenericValue(const T& value)
      : _value(value), _type(ECesiumMetadataValueType::None) {
    if (std::holds_alternative<int8_t>(_value) ||
        std::holds_alternative<uint8_t>(_value) ||
        std::holds_alternative<int16_t>(_value) ||
        std::holds_alternative<uint16_t>(_value) ||
        std::holds_alternative<int32_t>(_value) ||
        std::holds_alternative<uint32_t>(_value) ||
        std::holds_alternative<int64_t>(_value)) {
      _type = ECesiumMetadataValueType::Int64;
    }

    if (std::holds_alternative<uint64_t>(_value)) {
      _type = ECesiumMetadataValueType::Uint64;
    }

    if (std::holds_alternative<float>(_value)) {
      _type = ECesiumMetadataValueType::Float;
    }

    if (std::holds_alternative<double>(_value)) {
      _type = ECesiumMetadataValueType::Double;
    }

    if (std::holds_alternative<bool>(_value)) {
      _type = ECesiumMetadataValueType::Boolean;
    }

    if (std::holds_alternative<std::string_view>(_value)) {
      _type = ECesiumMetadataValueType::String;
    }

    _type = ECesiumMetadataValueType::Array;
  }

  ECesiumMetadataValueType GetType() const;

  int64 GetInt64() const;

  uint64 GetUint64() const;

  float GetFloat() const;

  double GetDouble() const;

  bool GetBoolean() const;

  FString GetString() const;

  FCesiumMetadataArray GetArray() const;

private:
  ValueType _value;
  ECesiumMetadataValueType _type;
};

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
  FCesiumMetadataProperty() 
    : _property(std::monostate{}), _type(ECesiumMetadataValueType::None)
  {}

  template<typename T>
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
    }

    if (holdsPropertyAlternative<uint64_t>(_property)) {
      _type = ECesiumMetadataValueType::Uint64;
    }

    if (holdsPropertyAlternative<float>(_property)) {
      _type = ECesiumMetadataValueType::Float;
    }

    if (holdsPropertyAlternative<double>(_property)) {
      _type = ECesiumMetadataValueType::Double;
    }

    if (holdsPropertyAlternative<bool>(_property)) {
      _type = ECesiumMetadataValueType::Boolean;
    }

    if (holdsPropertyAlternative<std::string_view>(_property)) {
      _type = ECesiumMetadataValueType::String;
    }

    _type = ECesiumMetadataValueType::Array;
  }

  ECesiumMetadataValueType GetType() const;

  bool GetBoolean(size_t featureID) const;

  int64 GetInt64(size_t featureID) const;

  uint64 GetUint64(size_t featureID) const;

  float GetFloat(size_t featureID) const;

  double GetDouble(size_t featureID) const;

  FString GetString(size_t featureID) const;

  FCesiumMetadataArray GetArray(size_t featureID) const;

private:
  template<typename T, typename ...VariantType>
  static bool holdsPropertyAlternative(const std::variant<VariantType...> &variant) {
    return std::holds_alternative<CesiumGltf::MetadataPropertyView<T>>(variant);
  }

  PropertyType _property;
  ECesiumMetadataValueType _type;
};

USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumFeatureTable {
  GENERATED_USTRUCT_BODY()

public:
  FCesiumFeatureTable(){};

  FCesiumFeatureTable(
      const CesiumGltf::Model& model,
      const CesiumGltf::FeatureTable& featureTable,
      const CesiumGltf::FeatureIDAttribute& featureIDAttribute);

  TMap<FString, FCesiumMetadataGenericValue>
  GetValuesForFeatureID(size_t featureID) const;

  FCesiumMetadataProperty GetProperty(const FString& name) const;

private:
  TMap<FString, FCesiumMetadataProperty> _properties;
};

//UCLASS()
//class CESIUMRUNTIME_API UCesiumMetadataBlueprintFunctionLibrary
//    : public UBlueprintFunctionLibrary {
//  GENERATED_BODY()
//
//  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
//  static TMap<FString, FCesiumMetadataGenericValue> GetMetadataForFeatureID(
//      UPARAM(ref) FCesiumMetadata& Metadata,
//      int64 FeatureID);
//
//  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
//  static FCesiumMetadataProperty
//  GetProperty(UPARAM(ref) FCesiumMetadata& Metadata, const FString& name);
//};
//
