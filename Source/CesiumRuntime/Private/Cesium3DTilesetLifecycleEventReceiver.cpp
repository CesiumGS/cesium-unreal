// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#include "Cesium3DTilesetLifecycleEventReceiver.h"

#include "Materials/MaterialInstanceDynamic.h"

UMaterialInstanceDynamic*
ICesium3DTilesetLifecycleEventReceiver::CreateMaterial(
    ICesiumLoadedTilePrimitive& TilePrim,
    UMaterialInterface* pBaseMaterial,
    const FName& Name) {
  // Default implementation: just create a new instance
  return UMaterialInstanceDynamic::Create(pBaseMaterial, nullptr, Name);
}

void ICesium3DTilesetLifecycleEventReceiver::CustomizeMaterial(
    ICesiumLoadedTilePrimitive& TilePrim,
    UMaterialInstanceDynamic&,
    const UCesiumMaterialUserData*,
    const CesiumGltf::Material&,
    const CesiumGltf::MaterialPBRMetallicRoughness&) {}

void ICesium3DTilesetLifecycleEventReceiver::OnTileMeshPrimitiveConstructed(
    ICesiumLoadedTilePrimitive& TilePrim) {}

void ICesium3DTilesetLifecycleEventReceiver::OnTileConstructed(
    ICesiumLoadedTile& LoadedTile) {}

void ICesium3DTilesetLifecycleEventReceiver::OnVisibilityChanged(
    ICesiumLoadedTile& LoadedTile,
    bool visible) {}

void ICesium3DTilesetLifecycleEventReceiver::BeforeTileDestruction(
    ICesiumLoadedTile& LoadedTile) {}
