#include "CesiumMetadata.h"
#include "CesiumGltf/MetadataFeatureTableView.h"

namespace {
struct MetadataArraySize {
  size_t operator()(std::monostate) { return 0; }

  template<typename T>
  size_t operator()(const CesiumGltf::MetadataArrayView<T>& value) { return value.size(); }
};

struct MetadataPropertyValue {
  FCesiumMetadataGenericValue operator()(std::monostate) {
    return FCesiumMetadataGenericValue{};
  }

  template<typename T>
  FCesiumMetadataGenericValue
  operator()(const CesiumGltf::MetadataPropertyView<T>& value) {
    return FCesiumMetadataGenericValue{value.get(featureID)};
  }

  size_t featureID;
};

template <typename T> struct IsNumericProperty {
  static constexpr bool value = false;
};
template <typename T> struct IsNumericProperty<CesiumGltf::MetadataPropertyView<T>> {
  static constexpr bool value = CesiumGltf::IsMetadataNumeric<T>::value;
};

template <typename T> struct IsArrayProperty {
  static constexpr bool value = false;
};
template <typename T> struct IsArrayProperty<CesiumGltf::MetadataPropertyView<T>> {
  static constexpr bool value = CesiumGltf::IsMetadataNumericArray<T>::value ||
                                CesiumGltf::IsMetadataBooleanArray<T>::value ||
                                CesiumGltf::IsMetadataStringArray<T>::value;
};
}

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

  const CesiumGltf::MetadataArrayView<std::string_view> &val =
      std::get<CesiumGltf::MetadataArrayView<std::string_view>>(_value);
  std::string_view member = val[i];
  return FString(member.size(), member.data());
}

ECesiumMetadataValueType FCesiumMetadataGenericValue::GetType() const {
  return _type;
}

int64 FCesiumMetadataGenericValue::GetInt64() const { 
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Int64),
      TEXT("Value cannot be represented as Int64"));

  return std::visit(
      [](const auto& v) -> int64_t {
        if constexpr (CesiumGltf::IsMetadataNumeric<decltype(v)>::value) {
          return static_cast<int64_t>(v);
        }
      },
      _value);
}

uint64 FCesiumMetadataGenericValue::GetUint64() const { 
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Uint64),
      TEXT("Value cannot be represented as Uint64"));
  return std::get<uint64_t>(_value);
}

float FCesiumMetadataGenericValue::GetFloat() const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Float),
      TEXT("Value cannot be represented as Float"));

  return std::get<float>(_value);
}

double FCesiumMetadataGenericValue::GetDouble() const { 
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Double),
      TEXT("Value cannot be represented as Double"));

  return std::get<double>(_value);
}

bool FCesiumMetadataGenericValue::GetBoolean() const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Boolean),
      TEXT("Value cannot be represented as Boolean"));

  return std::get<bool>(_value);
}

FString FCesiumMetadataGenericValue::GetString() const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::String),
      TEXT("Value cannot be represented as String"));

  std::string_view val = std::get<std::string_view>(_value);
  return FString(val.size(), val.data());
}

FCesiumMetadataArray FCesiumMetadataGenericValue::GetArray() const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Array),
      TEXT("Value cannot be represented as Array"));
  return std::visit(
      [](const auto& value) -> FCesiumMetadataArray {
        if constexpr (CesiumGltf::IsMetadataNumericArray<decltype(
                          value)>::value) {
          return FCesiumMetadataArray(value);
        }

        if constexpr (CesiumGltf::IsMetadataBooleanArray<decltype(
                          value)>::value) {
          return FCesiumMetadataArray(value);
        }

        if constexpr (CesiumGltf::IsMetadataStringArray<decltype(
                          value)>::value) {
          return FCesiumMetadataArray(value);
        }
      },
      _value);
}

ECesiumMetadataValueType FCesiumMetadataProperty::GetType() const {
  return _type;
}

int64 FCesiumMetadataProperty::GetInt64(size_t featureID) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Int64),
      TEXT("Value cannot be represented as Int64"));

  return std::visit(
      [featureID](const auto& v) -> int64_t {
        if constexpr (IsNumericProperty<decltype(v)>::value) {
          return static_cast<int64_t>(v.get(featureID));
        }
      },
      _property);
}

uint64 FCesiumMetadataProperty::GetUint64(size_t featureID) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Uint64),
      TEXT("Value cannot be represented as Uint64"));

  return std::get<CesiumGltf::MetadataPropertyView<float>>(_property).get(featureID);
}

float FCesiumMetadataProperty::GetFloat(size_t featureID) const { 
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Float),
      TEXT("Value cannot be represented as Float"));

  return std::get<CesiumGltf::MetadataPropertyView<float>>(_property).get(featureID);
}

double FCesiumMetadataProperty::GetDouble(size_t featureID) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Double),
      TEXT("Value cannot be represented as Double"));

  return std::get<CesiumGltf::MetadataPropertyView<double>>(_property).get(featureID);
}

bool FCesiumMetadataProperty::GetBoolean(size_t featureID) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Boolean),
      TEXT("Value cannot be represented as Boolean"));

  return std::get<CesiumGltf::MetadataPropertyView<bool>>(_property).get(featureID);
}

FString FCesiumMetadataProperty::GetString(size_t featureID) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::String),
      TEXT("Value cannot be represented as String"));

  std::string_view feature =
      std::get<CesiumGltf::MetadataPropertyView<std::string_view>>(_property)
          .get(featureID);

  return FString(feature.size(), feature.data());
}

FCesiumMetadataArray FCesiumMetadataProperty::GetArray(size_t featureID) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Array),
      TEXT("Value cannot be represented as Array"));

  return std::visit(
      [featureID](const auto& value) -> FCesiumMetadataArray {
        if constexpr (IsArrayProperty<decltype(value)>::value) {
          return FCesiumMetadataArray(value.get(featureID));
        }
      },
      _property);
}

FCesiumMetadataGenericValue
FCesiumMetadataProperty::GetGenericValue(size_t featureID) const {
  return std::visit(MetadataPropertyValue{featureID}, _property);
}

FCesiumMetadataFeatureTable::FCesiumMetadataFeatureTable(
    const CesiumGltf::Model& model,
    const CesiumGltf::FeatureTable& featureTable,
    const CesiumGltf::FeatureIDAttribute& featureIDAttribute) {
  CesiumGltf::MetadataFeatureTableView featureTableView{
      &model,
      &featureTable};

  featureTableView.forEachProperty([&properties = _properties](
                                       const std::string& propertyName,
                                       auto propertyValue) mutable {
    if (propertyValue) {
      FString key(propertyName.size(), propertyName.data());
      properties.Add(key, FCesiumMetadataProperty(*propertyValue));
    }
  });
}

TMap<FString, FCesiumMetadataGenericValue>
FCesiumMetadataFeatureTable::GetValuesForFeatureID(size_t featureID) const {
  TMap<FString, FCesiumMetadataGenericValue> feature;
  for (const auto pair : _properties) {
    feature.Add(pair.Key, pair.Value.GetGenericValue(featureID));
  }

  return feature;
}

FCesiumMetadataProperty
FCesiumMetadataFeatureTable::GetProperty(const FString& name) const {
  auto pProperty = _properties.Find(name);
  if (!pProperty) {
    return FCesiumMetadataProperty(); 
  }

  return *pProperty;
}

TMap<FString, FCesiumMetadataGenericValue>
UCesiumMetadataFeatureTableBlueprintLibrary::GetValuesForFeatureID(
    UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable,
    int64 featureID) {
  return featureTable.GetValuesForFeatureID(featureID);
}

FCesiumMetadataProperty UCesiumMetadataFeatureTableBlueprintLibrary::GetProperty(
    UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable,
    const FString& name) {
  return featureTable.GetProperty(name);
}

ECesiumMetadataValueType UCesiumMetadataPropertyBlueprintLibrary::GetType(
    UPARAM(ref) const FCesiumMetadataProperty& property) {
  return property.GetType();
}

bool UCesiumMetadataPropertyBlueprintLibrary::GetBoolean(
    UPARAM(ref) const FCesiumMetadataProperty& property,
    int64 featureID) {
  return property.GetBoolean(featureID);
}

int64 UCesiumMetadataPropertyBlueprintLibrary::GetInt64(
    UPARAM(ref) const FCesiumMetadataProperty& property,
    int64 featureID) {
  return property.GetInt64(featureID);
}

float UCesiumMetadataPropertyBlueprintLibrary::GetUint64AsFloat(
    UPARAM(ref) const FCesiumMetadataProperty& property,
    int64 featureID) {
  return static_cast<float>(property.GetUint64(featureID));
}

float UCesiumMetadataPropertyBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumMetadataProperty& property,
    int64 featureID) {
  return property.GetFloat(featureID);
}

float UCesiumMetadataPropertyBlueprintLibrary::GetDoubleAsFloat(
    UPARAM(ref) const FCesiumMetadataProperty& property,
    int64 featureID) {
  return static_cast<float>(property.GetDouble(featureID));
}

FString UCesiumMetadataPropertyBlueprintLibrary::GetString(
    UPARAM(ref) const FCesiumMetadataProperty& property,
    int64 featureID) {
  return property.GetString(featureID);
}

FCesiumMetadataArray UCesiumMetadataPropertyBlueprintLibrary::GetArray(
    UPARAM(ref) const FCesiumMetadataProperty& property,
    int64 featureID) {
  return property.GetArray(featureID);
}

FCesiumMetadataGenericValue
UCesiumMetadataPropertyBlueprintLibrary::GetGenericValue(
    UPARAM(ref) const FCesiumMetadataProperty& property,
    int64 featureID) {
  return property.GetGenericValue(featureID);
}

ECesiumMetadataValueType UCesiumMetadataGenericValueBlueprintLibrary::GetType(
    UPARAM(ref) const FCesiumMetadataGenericValue& value) {
  return value.GetType();
}

int64 UCesiumMetadataGenericValueBlueprintLibrary::GetInt64(
    UPARAM(ref) const FCesiumMetadataGenericValue& value) {
  return value.GetInt64();
}

float UCesiumMetadataGenericValueBlueprintLibrary::GetUint64AsFloat(
    UPARAM(ref) const FCesiumMetadataGenericValue& value) {
  return static_cast<float>(value.GetUint64());
}

float UCesiumMetadataGenericValueBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumMetadataGenericValue& value) {
  return value.GetFloat();
}

float UCesiumMetadataGenericValueBlueprintLibrary::GetDoubleAsFloat(
    UPARAM(ref) const FCesiumMetadataGenericValue& value) {
  return value.GetDouble();
}

bool UCesiumMetadataGenericValueBlueprintLibrary::GetBoolean(
    UPARAM(ref) const FCesiumMetadataGenericValue& value) {
  return value.GetBoolean();
}

FString UCesiumMetadataGenericValueBlueprintLibrary::GetString(
    UPARAM(ref) const FCesiumMetadataGenericValue& value) {
  return value.GetString();
}

FCesiumMetadataArray UCesiumMetadataGenericValueBlueprintLibrary::GetArray(
    UPARAM(ref) const FCesiumMetadataGenericValue& value) {
  return value.GetArray();
}

ECesiumMetadataValueType UCesiumMetadataArrayBlueprintLibrary::GetComponentType(
    UPARAM(ref) const FCesiumMetadataArray& array) {
  return array.GetComponentType();
}

bool UCesiumMetadataArrayBlueprintLibrary::GetBoolean(
    UPARAM(ref) const FCesiumMetadataArray& array,
    int64 index) {
  return array.GetBoolean(index);
}

int64 UCesiumMetadataArrayBlueprintLibrary::GetInt64(
    UPARAM(ref) const FCesiumMetadataArray& array,
    int64 index) {
  return array.GetInt64(index);
}

float UCesiumMetadataArrayBlueprintLibrary::GetUint64AsFloat(
    UPARAM(ref) const FCesiumMetadataArray& array,
    int64 index) {
  return static_cast<float>(array.GetUint64(index));
}

float UCesiumMetadataArrayBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumMetadataArray& array,
    int64 index) {
  return array.GetFloat(index);
}

float UCesiumMetadataArrayBlueprintLibrary::GetDoubleAsFloat(
    UPARAM(ref) const FCesiumMetadataArray& array,
    int64 index) {
  return static_cast<float>(array.GetDouble(index));
}

FString UCesiumMetadataArrayBlueprintLibrary::GetString(
    UPARAM(ref) const FCesiumMetadataArray& array,
    int64 index) {
  return array.GetString(index);
}

