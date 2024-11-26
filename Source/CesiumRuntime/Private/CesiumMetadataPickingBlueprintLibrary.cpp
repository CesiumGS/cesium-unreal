// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumMetadataPickingBlueprintLibrary.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumMetadataValue.h"

#include <optional>

static TMap<FString, FCesiumMetadataValue> EmptyCesiumMetadataValueMap;

TMap<FString, FCesiumMetadataValue>
UCesiumMetadataPickingBlueprintLibrary::GetMetadataValuesForFace(
    const UPrimitiveComponent* Component,
    int64 FaceIndex,
    int64 FeatureIDSetIndex) {
  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(Component);
  if (!IsValid(pGltfComponent)) {
    return TMap<FString, FCesiumMetadataValue>();
  }

  const UCesiumGltfComponent* pModel =
      Cast<UCesiumGltfComponent>(pGltfComponent->GetOuter());
  if (!IsValid(pModel)) {
    return TMap<FString, FCesiumMetadataValue>();
  }
  const CesiumPrimitiveData& primData = pGltfComponent->getPrimitiveData();
  const FCesiumPrimitiveFeatures& features = primData.Features;
  const TArray<FCesiumFeatureIdSet>& featureIDSets =
      UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(features);

  if (FeatureIDSetIndex < 0 || FeatureIDSetIndex >= featureIDSets.Num()) {
    return TMap<FString, FCesiumMetadataValue>();
  }

  const FCesiumFeatureIdSet& featureIDSet = featureIDSets[FeatureIDSetIndex];
  const int64 propertyTableIndex =
      UCesiumFeatureIdSetBlueprintLibrary::GetPropertyTableIndex(featureIDSet);

  const TArray<FCesiumPropertyTable>& propertyTables =
      UCesiumModelMetadataBlueprintLibrary::GetPropertyTables(pModel->Metadata);
  if (propertyTableIndex < 0 || propertyTableIndex >= propertyTables.Num()) {
    return TMap<FString, FCesiumMetadataValue>();
  }

  const FCesiumPropertyTable& propertyTable =
      propertyTables[propertyTableIndex];

  int64 featureID =
      UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
          features,
          FaceIndex,
          FeatureIDSetIndex);
  if (featureID < 0) {
    return TMap<FString, FCesiumMetadataValue>();
  }

  return UCesiumPropertyTableBlueprintLibrary::GetMetadataValuesForFeature(
      propertyTable,
      featureID);
}

TMap<FString, FString>
UCesiumMetadataPickingBlueprintLibrary::GetMetadataValuesForFaceAsStrings(
    const UPrimitiveComponent* Component,
    int64 FaceIndex,
    int64 FeatureIDSetIndex) {
  TMap<FString, FCesiumMetadataValue> values =
      UCesiumMetadataPickingBlueprintLibrary::GetMetadataValuesForFace(
          Component,
          FaceIndex,
          FeatureIDSetIndex);
  TMap<FString, FString> strings;
  for (auto valuesIt : values) {
    strings.Add(
        valuesIt.Key,
        UCesiumMetadataValueBlueprintLibrary::GetString(valuesIt.Value, ""));
  }

  return strings;
}

bool UCesiumMetadataPickingBlueprintLibrary::FindUVFromHit(
    const FHitResult& Hit,
    int64 GltfTexCoordSetIndex,
    FVector2D& UV) {
  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(Hit.Component);
  if (!IsValid(pGltfComponent)) {
    return false;
  }

  const CesiumPrimitiveData& primData = pGltfComponent->getPrimitiveData();

  if (primData.PositionAccessor.status() !=
      CesiumGltf::AccessorViewStatus::Valid) {
    return false;
  }

  auto accessorIt = primData.TexCoordAccessorMap.find(GltfTexCoordSetIndex);
  if (accessorIt == primData.TexCoordAccessorMap.end()) {
    return false;
  }

  auto VertexIndices = std::visit(
      CesiumGltf::IndicesForFaceFromAccessor{
          Hit.FaceIndex,
          primData.PositionAccessor.size(),
          primData.pMeshPrimitive->mode},
      primData.IndexAccessor);

  // Adapted from UBodySetup::CalcUVAtLocation. Compute the barycentric
  // coordinates of the point relative to the face, then use those to
  // interpolate the UVs.
  std::array<FVector2D, 3> UVs;
  const CesiumGltf::TexCoordAccessorType& accessor = accessorIt->second;
  for (size_t i = 0; i < UVs.size(); i++) {
    auto maybeTexCoord = std::visit(
        CesiumGltf::TexCoordFromAccessor{VertexIndices[i]},
        accessor);
    if (!maybeTexCoord) {
      return false;
    }
    const glm::dvec2& texCoord = *maybeTexCoord;
    UVs[i] = FVector2D(texCoord[0], texCoord[1]);
  }

  std::array<FVector, 3> Positions;
  for (size_t i = 0; i < Positions.size(); i++) {
    auto& Position = primData.PositionAccessor[VertexIndices[i]];
    // The Y-component of glTF positions must be inverted, and the positions
    // must be scaled to match the UE meshes.
    Positions[i] = FVector(Position[0], -Position[1], Position[2]) *
                   CesiumPrimitiveData::positionScaleFactor;
  }

  const FVector Location =
      pGltfComponent->GetComponentToWorld().InverseTransformPosition(
          Hit.Location);
  FVector BaryCoords = FMath::ComputeBaryCentric2D(
      Location,
      Positions[0],
      Positions[1],
      Positions[2]);

  UV = (BaryCoords.X * UVs[0]) + (BaryCoords.Y * UVs[1]) +
       (BaryCoords.Z * UVs[2]);

  return true;
}

namespace {
std::optional<TMap<FString, FCesiumMetadataValue>>
getInstancePropertyTableValues(const FHitResult& Hit, int64 FeatureIDSetIndex) {
  const auto* pInstancedComponent =
      Cast<UCesiumGltfInstancedComponent>(Hit.Component);
  if (!IsValid(pInstancedComponent)) {
    return std::nullopt;
  }
  const TSharedPtr<FCesiumInstanceFeatures> pInstanceFeatures =
      pInstancedComponent->pInstanceFeatures;
  if (!pInstanceFeatures) {
    return TMap<FString, FCesiumMetadataValue>();
  }

  const UCesiumGltfComponent* pModel =
      Cast<UCesiumGltfComponent>(pInstancedComponent->GetOuter());
  if (!IsValid(pModel)) {
    return TMap<FString, FCesiumMetadataValue>();
  }
  const FCesiumInstanceFeatures& instanceFeatures =
      UCesiumInstanceFeaturesBlueprintLibrary::GetInstanceFeatures(
          pInstancedComponent);
  const TArray<FCesiumFeatureIdSet>& featureIDSets =
      UCesiumInstanceFeaturesBlueprintLibrary::GetFeatureIDSets(
          instanceFeatures);
  if (FeatureIDSetIndex < 0 || FeatureIDSetIndex >= featureIDSets.Num()) {
    return TMap<FString, FCesiumMetadataValue>();
  }

  const FCesiumFeatureIdSet& featureIDSet = featureIDSets[FeatureIDSetIndex];
  const int64 propertyTableIndex =
      UCesiumFeatureIdSetBlueprintLibrary::GetPropertyTableIndex(featureIDSet);

  const TArray<FCesiumPropertyTable>& propertyTables =
      UCesiumModelMetadataBlueprintLibrary::GetPropertyTables(pModel->Metadata);
  if (propertyTableIndex < 0 || propertyTableIndex >= propertyTables.Num()) {
    return TMap<FString, FCesiumMetadataValue>();
  }
  const FCesiumPropertyTable& propertyTable =
      propertyTables[propertyTableIndex];
  int64 featureID =
      UCesiumInstanceFeaturesBlueprintLibrary::GetFeatureIDFromInstance(
          instanceFeatures,
          Hit.Item,
          FeatureIDSetIndex);
  if (featureID < 0) {
    return TMap<FString, FCesiumMetadataValue>();
  }
  return UCesiumPropertyTableBlueprintLibrary::GetMetadataValuesForFeature(
      propertyTable,
      featureID);
}
} // namespace

TMap<FString, FCesiumMetadataValue>
UCesiumMetadataPickingBlueprintLibrary::GetPropertyTableValuesFromHit(
    const FHitResult& Hit,
    int64 FeatureIDSetIndex) {
  std::optional<TMap<FString, FCesiumMetadataValue>> instanceProperties =
      getInstancePropertyTableValues(Hit, FeatureIDSetIndex);
  if (instanceProperties) {
    return *instanceProperties;
  }
  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(Hit.Component);
  if (!IsValid(pGltfComponent)) {
    return TMap<FString, FCesiumMetadataValue>();
  }

  const UCesiumGltfComponent* pModel =
      Cast<UCesiumGltfComponent>(pGltfComponent->GetOuter());
  if (!IsValid(pModel)) {
    return TMap<FString, FCesiumMetadataValue>();
  }

  const CesiumPrimitiveData& primData = pGltfComponent->getPrimitiveData();
  const FCesiumPrimitiveFeatures& features = primData.Features;
  const TArray<FCesiumFeatureIdSet>& featureIDSets =
      UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(features);

  if (FeatureIDSetIndex < 0 || FeatureIDSetIndex >= featureIDSets.Num()) {
    return TMap<FString, FCesiumMetadataValue>();
  }

  const FCesiumFeatureIdSet& featureIDSet = featureIDSets[FeatureIDSetIndex];
  const int64 propertyTableIndex =
      UCesiumFeatureIdSetBlueprintLibrary::GetPropertyTableIndex(featureIDSet);

  const TArray<FCesiumPropertyTable>& propertyTables =
      UCesiumModelMetadataBlueprintLibrary::GetPropertyTables(pModel->Metadata);
  if (propertyTableIndex < 0 || propertyTableIndex >= propertyTables.Num()) {
    return TMap<FString, FCesiumMetadataValue>();
  }

  const FCesiumPropertyTable& propertyTable =
      propertyTables[propertyTableIndex];

  int64 featureID =
      UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromHit(
          features,
          Hit,
          FeatureIDSetIndex);
  if (featureID < 0) {
    return TMap<FString, FCesiumMetadataValue>();
  }

  return UCesiumPropertyTableBlueprintLibrary::GetMetadataValuesForFeature(
      propertyTable,
      featureID);
}

TMap<FString, FCesiumMetadataValue>
UCesiumMetadataPickingBlueprintLibrary::GetPropertyTextureValuesFromHit(
    const FHitResult& Hit,
    int64 PropertyTextureIndex) {
  if (!Hit.Component.IsValid()) {
    return EmptyCesiumMetadataValueMap;
  }

  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(Hit.Component.Get());
  if (!IsValid(pGltfComponent)) {
    return EmptyCesiumMetadataValueMap;
  }

  const UCesiumGltfComponent* pModel =
      Cast<UCesiumGltfComponent>(pGltfComponent->GetOuter());
  if (!IsValid(pModel)) {
    return EmptyCesiumMetadataValueMap;
  }

  const TArray<FCesiumPropertyTexture>& propertyTextures =
      UCesiumModelMetadataBlueprintLibrary::GetPropertyTextures(
          pModel->Metadata);
  if (PropertyTextureIndex < 0 ||
      PropertyTextureIndex >= propertyTextures.Num()) {
    return EmptyCesiumMetadataValueMap;
  }

  return UCesiumPropertyTextureBlueprintLibrary::GetMetadataValuesFromHit(
      propertyTextures[PropertyTextureIndex],
      Hit);
}
