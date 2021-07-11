#include "CesiumMetadata.h"
#include "CesiumGltf/MetadataFeatureTableView.h"

namespace {
struct GenericValueToString {
  FString operator()(std::monostate) { return ""; }

  template <typename Integer> FString operator()(Integer value) {
    return FString::FromInt(value);
  }

  FString operator()(float value) { return FString::SanitizeFloat(value); }

  FString operator()(double value) { return FString::SanitizeFloat(value); }

  FString operator()(bool value) {
    if (value) {
      return "true";
    }

    return "false";
  }

  FString operator()(std::string_view value) {
    return FString(value.size(), value.data());
  }

  template <typename T>
  FString operator()(const CesiumGltf::MetadataArrayView<T>& value) {
    FString result = "{";
    FString seperator = "";
    for (size_t i = 0; i < value.size(); ++i) {
      result += seperator + (*this)(value[i]);
      seperator = ", ";
    }

    result += "}";
    return result;
  }
};

struct MetadataArraySize {
  size_t operator()(std::monostate) { return 0; }

  template <typename T>
  size_t operator()(const CesiumGltf::MetadataArrayView<T>& value) {
    return value.size();
  }
};

struct MetadataPropertySize {
  size_t operator()(std::monostate) { return 0; }

  template <typename T>
  size_t operator()(const CesiumGltf::MetadataPropertyView<T>& value) {
    return value.size();
  }
};

struct MetadataPropertyValue {
  FCesiumMetadataGenericValue operator()(std::monostate) {
    return FCesiumMetadataGenericValue{};
  }

  template <typename T>
  FCesiumMetadataGenericValue
  operator()(const CesiumGltf::MetadataPropertyView<T>& value) {
    return FCesiumMetadataGenericValue{value.get(featureID)};
  }

  size_t featureID;
};

struct FeatureIDFromAccessor {
  int64 operator()(std::monostate) { return -1; }

  template <typename T>
  int64 operator()(const CesiumGltf::AccessorView<T>& value) {
    return static_cast<int64>(value[vertexIdx].value[0]);
  }

  uint32_t vertexIdx;
};

template <typename T> struct IsNumericProperty {
  static constexpr bool value = false;
};
template <typename T>
struct IsNumericProperty<CesiumGltf::MetadataPropertyView<T>> {
  static constexpr bool value = CesiumGltf::IsMetadataNumeric<T>::value;
};

template <typename T> struct IsArrayProperty {
  static constexpr bool value = false;
};
template <typename T>
struct IsArrayProperty<CesiumGltf::MetadataPropertyView<T>> {
  static constexpr bool value = CesiumGltf::IsMetadataNumericArray<T>::value ||
                                CesiumGltf::IsMetadataBooleanArray<T>::value ||
                                CesiumGltf::IsMetadataStringArray<T>::value;
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

        assert(false && "Value cannot be represented as Int64");
        return 0;
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

        assert(false && "Value cannot be represented as Array");
        return FCesiumMetadataArray();
      },
      _value);
}

FString FCesiumMetadataGenericValue::ToString() const {
  return std::visit(GenericValueToString{}, _value);
}

ECesiumMetadataValueType FCesiumMetadataProperty::GetType() const {
  return _type;
}

size_t FCesiumMetadataProperty::GetNumOfFeatures() const {
  return std::visit(MetadataPropertySize{}, _property);
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

        assert(false && "Value cannot be represented as Int64");
        return 0;
      },
      _property);
}

uint64 FCesiumMetadataProperty::GetUint64(size_t featureID) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Uint64),
      TEXT("Value cannot be represented as Uint64"));

  return std::get<CesiumGltf::MetadataPropertyView<float>>(_property).get(
      featureID);
}

float FCesiumMetadataProperty::GetFloat(size_t featureID) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Float),
      TEXT("Value cannot be represented as Float"));

  return std::get<CesiumGltf::MetadataPropertyView<float>>(_property).get(
      featureID);
}

double FCesiumMetadataProperty::GetDouble(size_t featureID) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Double),
      TEXT("Value cannot be represented as Double"));

  return std::get<CesiumGltf::MetadataPropertyView<double>>(_property).get(
      featureID);
}

bool FCesiumMetadataProperty::GetBoolean(size_t featureID) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Boolean),
      TEXT("Value cannot be represented as Boolean"));

  return std::get<CesiumGltf::MetadataPropertyView<bool>>(_property).get(
      featureID);
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

        assert(false && "Value cannot be represented as Array");
        return FCesiumMetadataArray();
      },
      _property);
}

FCesiumMetadataGenericValue
FCesiumMetadataProperty::GetGenericValue(size_t featureID) const {
  return std::visit(MetadataPropertyValue{featureID}, _property);
}

FCesiumMetadataFeatureTable::FCesiumMetadataFeatureTable(
    const CesiumGltf::Model& model,
    const CesiumGltf::Accessor& featureIDAccessor,
    const CesiumGltf::FeatureTable& featureTable) {

  switch (featureIDAccessor.componentType) {
  case CesiumGltf::Accessor::ComponentType::BYTE:
    _featureIDAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<int8_t>>(
            model,
            featureIDAccessor);
    break;
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE:
    _featureIDAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint8_t>>(
            model,
            featureIDAccessor);
    break;
  case CesiumGltf::Accessor::ComponentType::SHORT:
    _featureIDAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<int16_t>>(
            model,
            featureIDAccessor);
    break;
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT:
    _featureIDAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint16_t>>(
            model,
            featureIDAccessor);
    break;
  case CesiumGltf::Accessor::ComponentType::FLOAT:
    _featureIDAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<float>>(
            model,
            featureIDAccessor);
    break;
  default:
    break;
  }

  CesiumGltf::MetadataFeatureTableView featureTableView{&model, &featureTable};

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
  for (const auto &pair : _properties) {
    feature.Add(pair.Key, pair.Value.GetGenericValue(featureID));
  }

  return feature;
}

TMap<FString, FString>
FCesiumMetadataFeatureTable::GetValuesAsStringsForFeatureID(
    size_t featureID) const {
  TMap<FString, FString> feature;
  for (const auto &pair : _properties) {
    feature.Add(pair.Key, pair.Value.GetGenericValue(featureID).ToString());
  }

  return feature;
}

size_t FCesiumMetadataFeatureTable::GetNumOfFeatures() const {
  if (_properties.Num() == 0) {
    return 0;
  }

  return _properties.begin().Value().GetNumOfFeatures();
}

int64 FCesiumMetadataFeatureTable::GetFeatureIDForVertex(
    uint32 vertexIdx) const {
  return std::visit(FeatureIDFromAccessor{vertexIdx}, _featureIDAccessor);
}

const TMap<FString, FCesiumMetadataProperty>&
FCesiumMetadataFeatureTable::GetProperties() const {
  return _properties;
}

FCesiumMetadataPrimitive::FCesiumMetadataPrimitive(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    const CesiumGltf::ModelEXT_feature_metadata& metadata,
    const CesiumGltf::MeshPrimitiveEXT_feature_metadata& primitiveMetadata) {
  for (const CesiumGltf::FeatureIDAttribute& attribute :
       primitiveMetadata.featureIdAttributes) {
    if (attribute.featureIds.attribute) {
      auto featureID =
          primitive.attributes.find(attribute.featureIds.attribute.value());
      if (featureID == primitive.attributes.end()) {
        continue;
      }

      const CesiumGltf::Accessor* accessor =
          model.getSafe<CesiumGltf::Accessor>(
              &model.accessors,
              featureID->second);
      if (!accessor) {
        continue;
      }

      if (accessor->type != CesiumGltf::Accessor::Type::SCALAR) {
        continue;
      }

      auto featureTable = metadata.featureTables.find(attribute.featureTable);
      if (featureTable == metadata.featureTables.end()) {
        continue;
      }

      _featureTables.Add((
          FCesiumMetadataFeatureTable(model, *accessor, featureTable->second)));
    }
  }
}

const TArray<FCesiumMetadataFeatureTable>&
FCesiumMetadataPrimitive::GetFeatureTables() const {
  return _featureTables;
}
