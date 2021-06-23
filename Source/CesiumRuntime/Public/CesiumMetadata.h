#pragma once

#include "CesiumGltf/MetadataFeatureTableView.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include <variant>
#include "CesiumMetadata.generated.h"

USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataArray {
  GENERATED_USTRUCT_BODY()

private:
  using ArrayType = std::variant<
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
  FCesiumMetadataValueType GetComponentType() const;

  size_t GetSize() const;

  bool GetBoolean(size_t i) const;

  int64 GetInteger(size_t i) const;

  uint64 GetUnsignedInteger(size_t i) const;

  float GetFloat(size_t i) const;

  double GetDouble(size_t i) const;

  void GetString(size_t i, const FChar **Str, size_t &StrLength) const;

private:
  ArrayType _value;
};

USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataGenericValue {
  GENERATED_USTRUCT_BODY()

private:
  using ValueType = std::variant<
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
  FCesiumMetadataValueType GetType() const;

  bool GetBoolean() const;

  int64 GetInteger() const;

  uint64 GetUnsignedInteger() const;

  float GetFloat() const;

  double GetDouble() const;

  void GetString(const FChar **Str, size_t &StrLength) const;

  FCesiumMetadataArray GetArray() const;

private:
  ValueType _value;
};

USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataProperty {
  GENERATED_USTRUCT_BODY()

private:
  using PropertyType = std::variant<
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
  FCesiumMetadataValueType GetType() const;

  bool GetBoolean() const;

  int64 GetInteger(size_t FeatureID) const;

  uint64 GetUnsignedInteger(size_t FeatureID) const;

  float GetFloat(size_t FeatureID) const;

  double GetDouble(size_t FeatureID) const;

  void GetString(size_t FeatureID, const FChar **Str, size_t &StrLength) const;

  FCesiumMetadataArray GetArray(size_t FeatureID) const;

private:
  PropertyType _property;
};

USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadata {
  GENERATED_USTRUCT_BODY()

  TMap<FString, FCesiumMetadataGenericValue>
  GetMetadataForFeatureID(size_t FeatureID) const;

  FCesiumMetadataProperty GetProperty(const FString& name) const;

private:
  TMap<FString, FCesiumMetadataProperty> _properties;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataBlueprintFunctionLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static TMap<FString, FCesiumMetadataGenericValue> GetMetadataForFeatureID(
      UPARAM(ref) FCesiumMetadata& Metadata,
      int64 FeatureID);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static FCesiumMetadataProperty
  GetProperty(UPARAM(ref) FCesiumMetadata& Metadata, const FString& name);
};

