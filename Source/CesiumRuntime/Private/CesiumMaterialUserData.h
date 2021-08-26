// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Engine/AssetUserData.h"
#include "CesiumMaterialUserData.generated.h"

/**
 * Instances of this user data class are automatically attached to all materials
 * that are used by Cesium for Unreal and that have a Layer Stack named
 * "Cesium". It provides a way for Cesium for Unreal to access the names of the
 * individual layers in the stack at runtime (i.e. outside the Editor) so that
 * they can be mapped to raster overlays.
 *
 * It works by responding, in the Editor, to changes in the Material to which
 * it's attached via the `PostEditChangeOwner` and updating its internal mirror
 * of the layer names. At runtime, these layer names that were configured in the
 * Editor can't be further changed, so the the mirrored list is still valid.
 */
UCLASS()
class UCesiumMaterialUserData : public UAssetUserData {
  GENERATED_BODY()

public:
  virtual void PostEditChangeOwner() override;

  UPROPERTY()
  TArray<FString> LayerNames;
};
