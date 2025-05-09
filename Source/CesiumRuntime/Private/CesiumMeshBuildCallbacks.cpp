/*--------------------------------------------------------------------------------------+
|
|     $Source: CesiumMeshBuildCallbacks.cpp $
|
|  $Copyright: (c) 2024 Bentley Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/

#include "CesiumMeshBuildCallbacks.h"

#include <Materials/MaterialInstanceDynamic.h>

CesiumMeshBuildCallbacks::CesiumMeshBuildCallbacks() {}

CesiumMeshBuildCallbacks::~CesiumMeshBuildCallbacks() {}

UMaterialInstanceDynamic* CesiumMeshBuildCallbacks::CreateMaterial(
    ICesiumLoadedTilePrimitive& TilePrim,
    UMaterialInterface*& InOut_pChosenBaseMaterial,
    UObject* InOuter,
    FName const& Name) {
  // Default implementation: just create a new instance
  return UMaterialInstanceDynamic::Create(
      InOut_pChosenBaseMaterial,
      InOuter,
      Name);
}

void CesiumMeshBuildCallbacks::CustomizeGltfMaterial(
    CesiumGltf::Material const& /*glTFmaterial*/,
    CesiumGltf::MaterialPBRMetallicRoughness const& /*pbr*/,
    UMaterialInstanceDynamic* /*pMaterial*/,
    EMaterialParameterAssociation /*association*/,
    int32 /*index*/) const {}
