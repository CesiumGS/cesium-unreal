// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataFeatureTable.h"
#include "CesiumGltf/MetadataFeatureTableView.h"
#include "CesiumMetadataPrimitive.h"

namespace {

struct FeatureIDFromAccessor {
  int64 operator()(std::monostate) { return -1; }

  int64 operator()(
      const CesiumGltf::AccessorView<AccessorTypes::SCALAR<float>>& value) {
    return static_cast<int64>(glm::round(value[vertexIdx].value[0]));
  }

  template <typename T>
  int64 operator()(const CesiumGltf::AccessorView<T>& value) {
    return static_cast<int64>(value[vertexIdx].value[0]);
  }

  int64 vertexIdx;
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
UCesiumMetadataFeatureTableBlueprintLibrary::GetMetadataValuesForFeatureID(
    UPARAM(ref) const FCesiumMetadataFeatureTable& FeatureTable,
    int64 featureID) {
  TMap<FString, FCesiumMetadataGenericValue> feature;
  for (const auto& pair : FeatureTable._properties) {
    feature.Add(
        pair.Key,
        UCesiumMetadataPropertyBlueprintLibrary::GetGenericValue(
            pair.Value,
            featureID));
  }

  return feature;
}

TMap<FString, FString>
UCesiumMetadataFeatureTableBlueprintLibrary::GetMetadataValuesAsStringForFeatureID(
    UPARAM(ref) const FCesiumMetadataFeatureTable& FeatureTable,
    int64 featureID) {
  TMap<FString, FString> feature;
  for (const auto& pair : FeatureTable._properties) {
    feature.Add(
        pair.Key,
        UCesiumMetadataGenericValueBlueprintLibrary::GetString(
            UCesiumMetadataPropertyBlueprintLibrary::GetGenericValue(
                pair.Value,
                featureID),
            ""));
  }

  return feature;
}

int64 UCesiumMetadataFeatureTableBlueprintLibrary::GetNumberOfFeatures(
    UPARAM(ref) const FCesiumMetadataFeatureTable& FeatureTable) {
  if (FeatureTable._properties.Num() == 0) {
    return 0;
  }

  return UCesiumMetadataPropertyBlueprintLibrary::GetNumberOfFeatures(
      FeatureTable._properties.begin().Value());
}

int64 UCesiumMetadataFeatureTableBlueprintLibrary::GetFeatureIDForVertex(
    UPARAM(ref) const FCesiumMetadataFeatureTable& FeatureTable,
    int64 vertexIdx) {
  return std::visit(
      FeatureIDFromAccessor{vertexIdx},
      FeatureTable._featureIDAccessor);
}

const TMap<FString, FCesiumMetadataProperty>&
UCesiumMetadataFeatureTableBlueprintLibrary::GetProperties(
    UPARAM(ref) const FCesiumMetadataFeatureTable& FeatureTable) {
  return FeatureTable._properties;
}
