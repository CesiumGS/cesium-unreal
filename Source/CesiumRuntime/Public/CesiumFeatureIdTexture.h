// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeatureTable.h"
#include "CesiumGltf/MeshFeaturesFeatureIdTextureView.h"
#include "Containers/UnrealString.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CesiumFeatureIdTexture.generated.h"

namespace CesiumGltf {
struct Model;
struct ExtensionExtMeshFeaturesFeatureIdTexture;
} // namespace CesiumGltf

/**
 * @brief A blueprint-accessible wrapper for a feature ID texture from a glTF
 * primitive. Provides access to per-pixel feature IDs, which can be used with
 * the corresponding {@link FCesiumFeatureTable} to access per-pixel metadata.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumFeatureIDTexture {
  GENERATED_USTRUCT_BODY()

public:
  FCesiumFeatureIDTexture() {}

  FCesiumFeatureIDTexture(
      const CesiumGltf::Model& model,
      const CesiumGltf::ExtensionExtMeshFeaturesFeatureIdTexture&
          featureIdTexture);

  constexpr const CesiumGltf::MeshFeatures::FeatureIdTextureView&
  getFeatureIDTextureView() const {
    return this->_featureIDTextureView;
  }

private:
  CesiumGltf::MeshFeatures::FeatureIdTextureView _featureIDTextureView;

  friend class UCesiumFeatureIDTextureBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumFeatureIDTextureBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * @brief Get the index of the texture coordinate set that corresponds to the
   * feature ID texture.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|FeatureIDTexture")
  static int64 GetTextureCoordinateIndex(
      const UPrimitiveComponent* component,
      UPARAM(ref) const FCesiumFeatureIDTexture& featureIDTexture);

  /**
   * @brief Given texture coordinates from the appropriate texture coordinate
   * set (as indicated by GetTextureCoordinateIndex), returns a feature ID
   * corresponding the pixel. The feature ID can be used with a
   * {@link FCesiumFeatureTable} to retrieve the per-vertex metadata.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|FeatureIDTexture")
  static int64 GetFeatureIDForTextureCoordinates(
      UPARAM(ref) const FCesiumFeatureIDTexture& featureIDTexture,
      float u,
      float v);
};
