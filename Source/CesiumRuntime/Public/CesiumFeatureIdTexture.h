// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/FeatureIDTextureView.h"
#include "CesiumMetadataFeatureTable.h"
#include "Containers/UnrealString.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CesiumFeatureIdTexture.generated.h"

namespace CesiumGltf {
struct Model;
struct FeatureIDTexture;
} // namespace CesiumGltf

USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumFeatureIdTexture {
  GENERATED_USTRUCT_BODY()

public:
  FCesiumFeatureIdTexture() {}

  FCesiumFeatureIdTexture(
      const CesiumGltf::Model& model,
      const CesiumGltf::FeatureIDTexture& featureIdTexture);

  constexpr const CesiumGltf::FeatureIDTextureView&
  getFeatureIdTextureView() const {
    return this->_featureIdTextureView;
  }

private:
  CesiumGltf::FeatureIDTextureView _featureIdTextureView;
  FString _featureTableName;

  friend class UCesiumFeatureIdTextureBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumFeatureIdTextureBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureIdTexture")
  static const FString&
  GetFeatureTableName(UPARAM(ref)
                          const FCesiumFeatureIdTexture& featureIdTexture);

  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureIdTexture")
  static int64 GetTextureCoordinateIndex(
      const UPrimitiveComponent* component,
      UPARAM(ref) const FCesiumFeatureIdTexture& featureIdTexture);

  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureIdTexture")
  static int64 GetFeatureIdForTextureCoordinates(
      UPARAM(ref) const FCesiumFeatureIdTexture& featureIdTexture,
      float u,
      float v);
};
