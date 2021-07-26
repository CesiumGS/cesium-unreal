#include "CesiumMetadataUtilityBlueprintLibrary.h"
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
