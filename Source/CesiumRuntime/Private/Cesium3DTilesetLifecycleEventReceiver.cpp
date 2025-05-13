// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#include "Cesium3DTilesetLifecycleEventReceiver.h"

#include "Materials/MaterialInstanceDynamic.h"

UMaterialInstanceDynamic*
ICesium3DTilesetLifecycleEventReceiver::CreateMaterial(
    ICesiumLoadedTilePrimitive& TilePrim,
    UMaterialInterface* pBaseMaterial,
    UObject* InOuter,
    const FName& Name) {
  // Default implementation: just create a new instance
  return UMaterialInstanceDynamic::Create(
      pBaseMaterial,
      InOuter,
      Name);
}

void ICesium3DTilesetLifecycleEventReceiver::CustomizeGltfMaterial(
    ICesiumLoadedTilePrimitive& TilePrim,
    const CesiumGltf::Material& /*glTFmaterial*/,
    const CesiumGltf::MaterialPBRMetallicRoughness& /*pbr*/,
    UMaterialInstanceDynamic* /*pMaterial*/,
    EMaterialParameterAssociation /*association*/,
    int32 /*index*/) {}
