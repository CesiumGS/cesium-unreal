// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumMetadataPickingBlueprintLibrary.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumMetadataValue.h"

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

  const FCesiumPrimitiveFeatures& features = pGltfComponent->Features;
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

namespace {
bool GetVertexIndicesFromFace(
    const FStaticMeshLODResources& LODResources,
    int32 FaceIndex,
    std::array<uint32, 3>& Indices) {
  int32 FirstIndex = FaceIndex * 3;

  if (LODResources.IndexBuffer.GetNumIndices() > 0) {
    if (FirstIndex + 2 >= LODResources.IndexBuffer.GetNumIndices()) {
      return false;
    }

    for (int32 i = 0; i < 3; i++) {
      Indices[static_cast<size_t>(i)] =
          LODResources.IndexBuffer.GetIndex(FirstIndex + i);
    }

    return true;
  }

  int32 VertexCount = LODResources.GetNumVertices();
  if (FirstIndex + 2 >= VertexCount) {
    return false;
  }

  Indices[0] = static_cast<uint32>(FirstIndex);
  Indices[1] = static_cast<uint32>(FirstIndex + 1);
  Indices[2] = static_cast<uint32>(FirstIndex + 2);

  return true;
}
} // namespace

bool UCesiumMetadataPickingBlueprintLibrary::FindUVFromHit(
    const FHitResult& Hit,
    int64 GltfTexCoordSetIndex,
    FVector2D& UV) {
  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(Hit.Component);
  if (!IsValid(pGltfComponent)) {
    return false;
  }

  const auto pMesh = pGltfComponent->GetStaticMesh();
  if (!IsValid(pMesh)) {
    return false;
  }

  const auto pRenderData = pMesh->GetRenderData();
  if (!pRenderData || pRenderData->LODResources.Num() == 0) {
    return false;
  }

  auto accessorIt =
      pGltfComponent->TexCoordAccessorMap.find(GltfTexCoordSetIndex);
  if (accessorIt == pGltfComponent->TexCoordAccessorMap.end()) {
    return false;
  }

  const auto& LODResources = pRenderData->LODResources[0];
  std::array<uint32, 3> VertexIndices;
  if (!GetVertexIndicesFromFace(LODResources, Hit.FaceIndex, VertexIndices)) {
    return false;
  }

  // Adapted from UBodySetup::CalcUVAtLocation. Compute the barycentric
  // coordinates of the point relative to the face, then use those to
  // interpolate the UVs.
  std::array<FVector2D, 3> UVs;
  const CesiumTexCoordAccessorType& accessor = accessorIt->second;
  for (size_t i = 0; i < UVs.size(); i++) {
    auto maybeTexCoord =
        std::visit(CesiumTexCoordFromAccessor{VertexIndices[i]}, accessor);
    if (!maybeTexCoord) {
      return false;
    }

    const glm::dvec2& texCoord = *maybeTexCoord;
    UVs[i] = FVector2D(texCoord[0], texCoord[1]);
  }

  std::array<FVector, 3> Positions;
  for (size_t i = 0; i < Positions.size(); i++) {
    auto Position =
        LODResources.VertexBuffers.PositionVertexBuffer.VertexPosition(
            VertexIndices[i]);
    Positions[i] = FVector(Position[0], Position[1], Position[2]);
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

TMap<FString, FCesiumMetadataValue>
UCesiumMetadataPickingBlueprintLibrary::GetPropertyTableValuesFromHit(
    const FHitResult& Hit,
    int64 FeatureIDSetIndex) {
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

  const FCesiumPrimitiveFeatures& features = pGltfComponent->Features;
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
          Hit.FaceIndex,
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
