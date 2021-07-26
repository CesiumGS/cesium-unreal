// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataArray.h"
#include "CesiumGltf/PropertyTypeTraits.h"

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

int64 FCesiumMetadataArray::GetInt64(size_t i) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Int64),
      TEXT("Value cannot be represented as Int64"));

  return std::visit(
      [i](const auto& v) -> int64_t {
        if constexpr (CesiumGltf::IsMetadataNumeric<decltype(v)>::value) {
          return static_cast<int64_t>(v[i]);
        }

        assert(false && "Value cannot be represented as Int64");
        return 0;
      },
      _value);
}

uint64 FCesiumMetadataArray::GetUint64(size_t i) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Uint64),
      TEXT("Value cannot be represented as Uint64"));

  return std::get<CesiumGltf::MetadataArrayView<uint64_t>>(_value)[i];
}

float FCesiumMetadataArray::GetFloat(size_t i) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Float),
      TEXT("Value cannot be represented as Float"));

  return std::get<CesiumGltf::MetadataArrayView<float>>(_value)[i];
}

double FCesiumMetadataArray::GetDouble(size_t i) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Double),
      TEXT("Value cannot be represented as Double"));

  return std::get<CesiumGltf::MetadataArrayView<double>>(_value)[i];
}

bool FCesiumMetadataArray::GetBoolean(size_t i) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Boolean),
      TEXT("Value cannot be represented as Boolean"));

  return std::get<CesiumGltf::MetadataArrayView<bool>>(_value)[i];
}

FString FCesiumMetadataArray::GetString(size_t i) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::String),
      TEXT("Value cannot be represented as String"));

  const CesiumGltf::MetadataArrayView<std::string_view>& val =
      std::get<CesiumGltf::MetadataArrayView<std::string_view>>(_value);
  std::string_view member = val[i];
  return FString(member.size(), member.data());
}
