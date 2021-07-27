// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataArray.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumMetadataConversions.h"

namespace {

struct MetadataArraySize {
  size_t operator()(std::monostate) { return 0; }

  template <typename T>
  size_t operator()(const CesiumGltf::MetadataArrayView<T>& value) {
    return value.size();
  }
};

} // namespace

ECesiumMetadataValueType FCesiumMetadataArray::GetComponentType() const {
  return _type;
}

size_t FCesiumMetadataArray::GetSize() const {
  return std::visit(MetadataArraySize{}, _value);
}

bool FCesiumMetadataArray::GetBoolean(int64 index, bool defaultValue) const {
  return std::visit(
      [index, defaultValue](const auto& v) -> bool {
        auto value = v[index];
        return MetadataConverter<bool, decltype(value)>::convert(
            value,
            defaultValue);
      },
      _value);
}

uint8 FCesiumMetadataArray::GetByte(int64 index, uint8 defaultValue) const {
  return std::visit(
      [index, defaultValue](const auto& v) -> uint8 {
        auto value = v[index];
        return MetadataConverter<uint8, decltype(value)>::convert(
            value,
            defaultValue);
      },
      _value);
}

int32 FCesiumMetadataArray::GetInteger(int64 index, int32 defaultValue) const {
  return std::visit(
      [index, defaultValue](const auto& v) -> int32 {
        auto value = v[index];
        return MetadataConverter<int32, decltype(value)>::convert(
            value,
            defaultValue);
      },
      _value);
}

int64 FCesiumMetadataArray::GetInteger64(int64 index, int64 defaultValue)
    const {
  return std::visit(
      [index, defaultValue](const auto& v) -> int64 {
        auto value = v[index];
        return MetadataConverter<int64, decltype(value)>::convert(
            value,
            defaultValue);
      },
      _value);
}

float FCesiumMetadataArray::GetFloat(int64 index, float defaultValue) const {
  return std::visit(
      [index, defaultValue](const auto& v) -> float {
        auto value = v[index];
        return MetadataConverter<float, decltype(value)>::convert(
            value,
            defaultValue);
      },
      _value);
}

FString FCesiumMetadataArray::GetString(
    int64 index,
    const FString& defaultValue) const {
  return std::visit(
      [index, defaultValue](const auto& v) -> FString {
        auto value = v[index];
        return MetadataConverter<FString, decltype(value)>::convert(
            value,
            defaultValue);
      },
      _value);
}

ECesiumMetadataValueType UCesiumMetadataArrayBlueprintLibrary::GetComponentType(
    UPARAM(ref) const FCesiumMetadataArray& array) {
  return array.GetComponentType();
}

int64 UCesiumMetadataArrayBlueprintLibrary::GetSize(
    UPARAM(ref) const FCesiumMetadataArray& array) {
  return array.GetSize();
}

bool UCesiumMetadataArrayBlueprintLibrary::GetBoolean(
    UPARAM(ref) const FCesiumMetadataArray& Array,
    int64 Index,
    bool DefaultValue) {
  return Array.GetBoolean(Index, DefaultValue);
}

uint8 UCesiumMetadataArrayBlueprintLibrary::GetByte(
    UPARAM(ref) const FCesiumMetadataArray& Array,
    int64 Index,
    uint8 DefaultValue) {
  return Array.GetByte(Index, DefaultValue);
}

int32 UCesiumMetadataArrayBlueprintLibrary::GetInteger(
    UPARAM(ref) const FCesiumMetadataArray& Array,
    int64 Index,
    int32 DefaultValue) {
  return Array.GetInteger(Index, DefaultValue);
}

int64 UCesiumMetadataArrayBlueprintLibrary::GetInteger64(
    UPARAM(ref) const FCesiumMetadataArray& Array,
    int64 Index,
    int64 DefaultValue) {
  return Array.GetInteger64(Index, DefaultValue);
}

float UCesiumMetadataArrayBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumMetadataArray& Array,
    int64 Index,
    float DefaultValue) {
  return Array.GetFloat(Index, DefaultValue);
}

FString UCesiumMetadataArrayBlueprintLibrary::GetString(
    UPARAM(ref) const FCesiumMetadataArray& Array,
    int64 Index,
    const FString& DefaultValue) {
  return Array.GetString(Index, DefaultValue);
}
