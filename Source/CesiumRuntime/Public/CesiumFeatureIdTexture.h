// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeatureTable.h"
#include "CesiumGltf/FeatureIDTextureView.h"
#include "Containers/UnrealString.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CesiumFeatureIdTexture.generated.h"

namespace CesiumGltf {
struct Model;
struct FeatureIDTexture;
} // namespace CesiumGltf

/**
 * @brief A blueprint-accessible wrapper for a feature id texture from a glTF
 * primitive. Provides access to per-pixel feature IDs which can be used with
 * the corresponding {@link FCesiumFeatureTable} to access per-pixel metadata.
 */
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
  /**
   * @brief Get the name of the feature table corresponding to this feature ID
   * texture. The name can be used to fetch the appropriate
   * {@link FCesiumFeatureTable} from the {@link FCesiumMetadataModel}.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureIdTexture")
  static const FString&
  GetFeatureTableName(UPARAM(ref)
                          const FCesiumFeatureIdTexture& featureIdTexture);

  /**
   * @brief Get the index of the texture coordinate set that corresponds to the
   * feature id texture.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureIdTexture")
  static int64 GetTextureCoordinateIndex(
      const UPrimitiveComponent* component,
      UPARAM(ref) const FCesiumFeatureIdTexture& featureIdTexture);

  /**
   * @brief Given texture coordinates from the appropriate texture coordinate
   * set (as indicated by GetTextureCoordinateIndex), returns a feature ID
   * corresponding the pixel. The feature ID can be used with a
   * {@link FCesiumFeatureTable} to retrieve the per-vertex metadata.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureIdTexture")
  static int64 GetFeatureIdForTextureCoordinates(
      UPARAM(ref) const FCesiumFeatureIdTexture& featureIdTexture,
      float u,
      float v);
};
