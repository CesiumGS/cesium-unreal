// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/FeatureIDTextureView.h"
#include "CesiumMetadataFeatureTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CesiumFeatureIDTexture.generated.h"

namespace CesiumGltf {
struct Model;
struct FeatureIDTexture;
} // namespace CesiumGltf

USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumFeatureIDTexture {
  GENERATED_USTRUCT_BODY()

public:
  FCesiumFeatureIDTexture() {}

  FCesiumFeatureIDTexture(
      const CesiumGltf::Model& model,
      const CesiumGltf::FeatureIDTexture& featureIDTexture);

private:
  CesiumGltf::FeatureIDTextureView _featureIDTextureView;
  FCesiumMetadataFeatureTable _featureTable;

  friend class UCesiumFeatureIDTextureBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumFeatureIDTextureBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureIDTexture")
  static const FCesiumMetadataFeatureTable&
  GetFeatureTable(UPARAM(ref) const FCesiumFeatureIDTexture& featureIDTexture);

  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureIDTexture")
  static int64 GetTextureCoordinateIndex(
      UPARAM(ref) const FCesiumFeatureIDTexture& featureIDTexture);

  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureIDTexture")
  static int64 GetFeatureIDForTextureCoordinates(
      UPARAM(ref) const FCesiumFeatureIDTexture& featureIDTexture,
      float u,
      float v);
};
