#include "CesiumMetadataBlueprintLibrary.h"
#include "CesiumGltfPrimitiveComponent.h"

namespace {
struct GetVertexIndexForFace {
  template <typename T> void operator()(const T& view) { return; }

  template <typename T>
  void operator()(
      const CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<T>>&
          view) {
    int64_t vertexBegin = faceID * 3;
    if (vertexBegin + 3 >= view.size()) {
      return;
    }

    *v0 = static_cast<int64_t>(view[vertexBegin].value[0]);
    *v1 = static_cast<int64_t>(view[vertexBegin + 1].value[0]);
    *v2 = static_cast<int64_t>(view[vertexBegin + 2].value[0]);

    return;
  }

  int64_t faceID{};
  int64_t* v0{};
  int64_t* v1{};
  int64_t* v2{};
};

int64 GetFeatureIDForFace(
    int64_t faceID,
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& meshPrimitive,
    const FCesiumMetadataFeatureTable& featureTable) {
  if (faceID < 0) {
    return -1;
  }

  int64_t v0 = -1;
  int64_t v1 = -1;
  int64_t v2 = -1;

  CesiumGltf::createAccessorView(
      model,
      meshPrimitive.indices,
      GetVertexIndexForFace{faceID, &v0, &v1, &v2});

  if (v0 < 0 || v1 < 0 || v2 < 0) {
    return -1;
  }

  int64_t id0 = featureTable.GetFeatureIDForVertex(v0);
  int64_t id1 = featureTable.GetFeatureIDForVertex(v1);
  int64_t id2 = featureTable.GetFeatureIDForVertex(v2);

  if (id0 != id1 || id0 != id2 || id1 != id2) {
    return -1;
  }

  return id0;
}
} // namespace

int64 UCesiumMetadataFeatureTableBlueprintLibrary::GetNumOfFeatures(
    UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable) {
  return featureTable.GetNumOfFeatures();
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

ECesiumMetadataValueType UCesiumMetadataPropertyBlueprintLibrary::GetType(
    UPARAM(ref) const FCesiumMetadataProperty& property) {
  return property.GetType();
}

int64 UCesiumMetadataPropertyBlueprintLibrary::GetNumOfFeatures(
    UPARAM(ref) const FCesiumMetadataProperty& property) {
  return property.GetNumOfFeatures();
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

const TArray<FCesiumMetadataFeatureTable>&
UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureTables(
    const FCesiumMetadataPrimitive& metadataPrimitive) {
  return metadataPrimitive.GetFeatureTables();
}

FCesiumMetadataPrimitive
UCesiumMetadataUtilityBlueprintLibrary::GetPrimitiveMetadata(
    const UPrimitiveComponent* component) {
  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(component);
  if (!IsValid(pGltfComponent)) {
    return FCesiumMetadataPrimitive();
  }

  return pGltfComponent->Metadata;
}

TMap<FString, FCesiumMetadataGenericValue>
UCesiumMetadataUtilityBlueprintLibrary::GetMetadataValuesForFace(
    const UPrimitiveComponent* component,
    int64 faceID) {
  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(component);
  if (!IsValid(pGltfComponent)) {
    return TMap<FString, FCesiumMetadataGenericValue>();
  }

  const FCesiumMetadataPrimitive& metadata = pGltfComponent->Metadata;
  const TArray<FCesiumMetadataFeatureTable>& featureTables =
      metadata.GetFeatureTables();
  if (featureTables.Num() == 0) {
    return TMap<FString, FCesiumMetadataGenericValue>();
  }

  const FCesiumMetadataFeatureTable& featureTable = featureTables[0];
  int64 featureID = GetFeatureIDForFace(
      faceID,
      *pGltfComponent->pModel,
      *pGltfComponent->pMeshPrimitive,
      featureTable);
  if (featureID < 0) {
    return TMap<FString, FCesiumMetadataGenericValue>();
  }

  return featureTable.GetValuesForFeatureID(featureID);
}

TMap<FString, FString>
UCesiumMetadataUtilityBlueprintLibrary::GetMetadataValuesAsStringForFace(
    const UPrimitiveComponent* component,
    int64 faceID) {
  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(component);
  if (!IsValid(pGltfComponent)) {
    return TMap<FString, FString>();
  }

  const FCesiumMetadataPrimitive& metadata = pGltfComponent->Metadata;
  const TArray<FCesiumMetadataFeatureTable>& featureTables =
      metadata.GetFeatureTables();
  if (featureTables.Num() == 0) {
    return TMap<FString, FString>();
  }

  const FCesiumMetadataFeatureTable& featureTable = featureTables[0];
  int64 featureID = GetFeatureIDForFace(
      faceID,
      *pGltfComponent->pModel,
      *pGltfComponent->pMeshPrimitive,
      featureTable);
  if (featureID < 0) {
    return TMap<FString, FString>();
  }

  return featureTable.GetValuesAsStringsForFeatureID(featureID);
}
