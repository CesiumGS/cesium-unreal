// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataFeatureTable.h"
#include "CesiumGltf/MetadataFeatureTableView.h"

namespace {

struct FeatureIDFromAccessor {
  int64 operator()(std::monostate) { return -1; }

  template <typename T>
  int64 operator()(const CesiumGltf::AccessorView<T>& value) {
    return static_cast<int64>(value[vertexIdx].value[0]);
  }

  uint32_t vertexIdx;
};

} // namespace

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
    if (propertyValue.status() == MetadataPropertyViewStatus::Valid) {
      FString key(propertyName.size(), propertyName.data());
      properties.Add(key, FCesiumMetadataProperty(propertyValue));
    }
  });
}

TMap<FString, FCesiumMetadataGenericValue>
FCesiumMetadataFeatureTable::GetValuesForFeatureID(size_t featureID) const {
  TMap<FString, FCesiumMetadataGenericValue> feature;
  for (const auto& pair : _properties) {
    feature.Add(pair.Key, pair.Value.GetGenericValue(featureID));
  }

  return feature;
}

TMap<FString, FString>
FCesiumMetadataFeatureTable::GetValuesAsStringsForFeatureID(
    size_t featureID) const {
  TMap<FString, FString> feature;
  for (const auto& pair : _properties) {
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

int64 UCesiumMetadataFeatureTableBlueprintLibrary::GetNumOfFeatures(
    UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable) {
  return featureTable.GetNumOfFeatures();
}

int64 UCesiumMetadataFeatureTableBlueprintLibrary::GetFeatureIDForVertex(
    UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable,
    int64 vertexIdx) {
  return featureTable.GetFeatureIDForVertex(vertexIdx);
}

TMap<FString, FCesiumMetadataGenericValue>
UCesiumMetadataFeatureTableBlueprintLibrary::GetValuesForFeatureID(
    UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable,
    int64 featureID) {
  return featureTable.GetValuesForFeatureID(featureID);
}

TMap<FString, FString>
UCesiumMetadataFeatureTableBlueprintLibrary::GetValuesAsStringsForFeatureID(
    UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable,
    int64 featureID) {
  return featureTable.GetValuesAsStringsForFeatureID(featureID);
}

const TMap<FString, FCesiumMetadataProperty>&
UCesiumMetadataFeatureTableBlueprintLibrary::GetProperties(
    UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable) {
  return featureTable.GetProperties();
}
