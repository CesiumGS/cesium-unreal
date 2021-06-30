#pragma once

#include "CesiumGltf/FeatureIDAttribute.h"
#include "CesiumGltf/MetadataArrayView.h"
#include "CesiumGltf/MetadataPropertyView.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/ModelEXT_feature_metadata.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
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
      : _value(std::monostate{}), _type(ECesiumMetadataValueType::None) {}

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

  ECesiumMetadataValueType GetComponentType() const;

  size_t GetSize() const;

  int64 GetInt64(size_t i) const;

  uint64 GetUint64(size_t i) const;

  float GetFloat(size_t i) const;

  double GetDouble(size_t i) const;

  bool GetBoolean(size_t i) const;

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
      : _value(std::monostate{}), _type(ECesiumMetadataValueType::None) {}

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
      : _property(std::monostate{}), _type(ECesiumMetadataValueType::None) {}

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

  ECesiumMetadataValueType GetType() const;

  bool GetBoolean(size_t featureID) const;

  int64 GetInt64(size_t featureID) const;

  uint64 GetUint64(size_t featureID) const;

  float GetFloat(size_t featureID) const;

  double GetDouble(size_t featureID) const;

  FString GetString(size_t featureID) const;

  FCesiumMetadataArray GetArray(size_t featureID) const;

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

USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataFeatureTable {
  GENERATED_USTRUCT_BODY()

public:
  FCesiumMetadataFeatureTable(){};

  FCesiumMetadataFeatureTable(
      const CesiumGltf::Model& model,
      const CesiumGltf::FeatureTable& featureTable);

  TMap<FString, FCesiumMetadataGenericValue>
  GetValuesForFeatureID(size_t featureID) const;

  FCesiumMetadataProperty GetProperty(const FString& name) const;

private:
  TMap<FString, FCesiumMetadataProperty> _properties;
};

USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadata {
  GENERATED_USTRUCT_BODY()

public:
  FCesiumMetadata() {}

  FCesiumMetadata(
      const CesiumGltf::Model& model,
      const CesiumGltf::ModelEXT_feature_metadata& metadata);

  const FString &GetSchemaName() const;

  const FString &GetSchemaDescription() const;

  const FString &GetVersion() const;

  const TMap<FString, FCesiumMetadataFeatureTable>& GetFeatureTables() const;

private:
  FString _schemaName;
  FString _schemaDescription;
  FString _schemaVersion;
  TMap<FString, FCesiumMetadataFeatureTable> _featureTables;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataArrayBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static ECesiumMetadataValueType
  GetComponentType(UPARAM(ref) const FCesiumMetadataArray& array);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static bool
  GetBoolean(UPARAM(ref) const FCesiumMetadataArray& array, int64 index);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static int64
  GetInt64(UPARAM(ref) const FCesiumMetadataArray& array, int64 index);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static float
  GetUint64AsFloat(UPARAM(ref) const FCesiumMetadataArray& array, int64 index);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static float
  GetFloat(UPARAM(ref) const FCesiumMetadataArray& array, int64 index);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static float
  GetDoubleAsFloat(UPARAM(ref) const FCesiumMetadataArray& array, int64 index);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static FString
  GetString(UPARAM(ref) const FCesiumMetadataArray& array, int64 index);
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataGenericValueBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static ECesiumMetadataValueType
  GetType(UPARAM(ref) const FCesiumMetadataGenericValue& value);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static int64 GetInt64(UPARAM(ref) const FCesiumMetadataGenericValue& value);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static float GetUint64AsFloat(UPARAM(ref)
                                    const FCesiumMetadataGenericValue& value);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static float GetFloat(UPARAM(ref) const FCesiumMetadataGenericValue& value);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static float GetDoubleAsFloat(UPARAM(ref)
                                    const FCesiumMetadataGenericValue& value);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static bool GetBoolean(UPARAM(ref) const FCesiumMetadataGenericValue& value);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static FString GetString(UPARAM(ref)
                               const FCesiumMetadataGenericValue& value);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static FCesiumMetadataArray
  GetArray(UPARAM(ref) const FCesiumMetadataGenericValue& value);
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataPropertyBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static ECesiumMetadataValueType
  GetType(UPARAM(ref) const FCesiumMetadataProperty& property);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static bool GetBoolean(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static int64 GetInt64(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static float GetUint64AsFloat(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static float GetFloat(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static float GetDoubleAsFloat(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static FString GetString(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static FCesiumMetadataArray GetArray(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static FCesiumMetadataGenericValue GetGenericValue(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataFeatureTableBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static TMap<FString, FCesiumMetadataGenericValue> GetValuesForFeatureID(
      UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable,
      int64 featureID);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static FCesiumMetadataProperty GetProperty(
      UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable,
      const FString& name);
};
