// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#include "Cesium3DTilesetLifecycleEventReceiver.h"

#include "Materials/MaterialInstanceDynamic.h"

UMaterialInstanceDynamic*
ICesium3DTilesetLifecycleEventReceiver::CreateMaterial(
    ICesiumLoadedTilePrimitive& TilePrimitive,
    UMaterialInterface* DefaultBaseMaterial,
    const FName& Name) {
  // Default implementation: just create a new instance
  return UMaterialInstanceDynamic::Create(DefaultBaseMaterial, nullptr, Name);
}

void ICesium3DTilesetLifecycleEventReceiver::CustomizeMaterial(
    ICesiumLoadedTilePrimitive& TilePrimitive,
    UMaterialInstanceDynamic& Material,
    const UCesiumMaterialUserData* CesiumData,
    const CesiumGltf::Material& GlTFmaterial,
    const CesiumGltf::MaterialPBRMetallicRoughness& GlTFmaterialPBR) {}

void ICesium3DTilesetLifecycleEventReceiver::OnTileMeshPrimitiveLoaded(
    ICesiumLoadedTilePrimitive& TilePrimitive) {}

void ICesium3DTilesetLifecycleEventReceiver::OnTileLoaded(
    ICesiumLoadedTile& Tile) {}

void ICesium3DTilesetLifecycleEventReceiver::OnTileVisibilityChanged(
    ICesiumLoadedTile& Tile,
    bool bVisible) {}

void ICesium3DTilesetLifecycleEventReceiver::OnTileUnloading(
    ICesiumLoadedTile& Tile) {}
