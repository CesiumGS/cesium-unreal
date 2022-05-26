// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeatureTextureProperty.h"
#include "CesiumGltf/FeatureTextureView.h"
#include "Containers/Array.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CesiumFeatureTexture.generated.h"

namespace CesiumGltf {
struct Model;
struct FeatureTexture;
}; // namespace CesiumGltf

/**
 * @brief A blueprint-accessible wrapper of a feature texture from a glTF.
 * Provides access to {@link FCesiumFeatureTextureProperty} views of texture
 * metadata.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumFeatureTexture {
  GENERATED_USTRUCT_BODY()

public:
  FCesiumFeatureTexture() {}

  FCesiumFeatureTexture(
      const CesiumGltf::Model& model,
      const CesiumGltf::FeatureTexture& featureTexture);

  const CesiumGltf::FeatureTextureView& getFeatureTextureView() const {
    return this->_featureTextureView;
  }

private:
  CesiumGltf::FeatureTextureView _featureTextureView;
  TArray<FString> _propertyKeys;

  friend class UCesiumFeatureTextureBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumFeatureTextureBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * @brief Gets an array of property names corresponding to
   * {@link FCesiumFeatureTextureProperty} views. These property names can be
   * used with FindProperty to retrieve the corresponding
   * {@link FCesiumFeatureTextureProperty} views.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTexture")
  static const TArray<FString>&
  GetPropertyKeys(UPARAM(ref) const FCesiumFeatureTexture& FeatureTexture);

  /**
   * @brief Retrieve a {@link FCesiumFeatureTextureProperty} by name. Use
   * GetPropertyKeys to get a list of names for existing feature texture
   * properties.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTexture")
  static FCesiumFeatureTextureProperty FindProperty(
      UPARAM(ref) const FCesiumFeatureTexture& FeatureTexture,
      const FString& PropertyName);
};
