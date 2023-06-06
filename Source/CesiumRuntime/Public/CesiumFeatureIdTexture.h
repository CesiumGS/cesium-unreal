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
struct CESIUMRUNTIME_API FCesiumFeatureIdTexture {
  GENERATED_USTRUCT_BODY()

  using TexCoordAccessorType = std::variant<
      std::monostate,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::VEC2<uint8_t>>,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::VEC2<uint16_t>>,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::VEC2<float>>>;

public:
  FCesiumFeatureIdTexture() {}

  FCesiumFeatureIdTexture(
      const CesiumGltf::Model& InModel,
      const CesiumGltf::MeshPrimitive Primitive,
      const CesiumGltf::ExtensionExtMeshFeaturesFeatureIdTexture&
          FeatureIdTexture);

  constexpr const CesiumGltf::MeshFeatures::FeatureIdTextureView&
  getFeatureIdTextureView() const {
    return this->_featureIdTextureView;
  }

private:
  CesiumGltf::MeshFeatures::FeatureIdTextureView _featureIdTextureView;

  TexCoordAccessorType _texCoordAccessor;
  int64 _textureCoordinateIndex;

  friend class UCesiumFeatureIdTextureBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumFeatureIdTextureBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Get the index of the texture coordinate set that corresponds to the
   * feature ID texture.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|FeatureIDTexture")
  static int64 GetTextureCoordinateIndex(
      const UPrimitiveComponent* Component,
      UPARAM(ref) const FCesiumFeatureIdTexture& FeatureIDTexture);

  /**
   * Gets the feature ID corresponding to the pixel specified by the texture
   * coordinates. The feature ID can be used with a FCesiumFeatureTable to
   * retrieve the per-pixel metadata.
   *
   * This assumes the given texture coordinates are from the appropriate
   * texture coordinate set as indicated by GetTextureCoordinateIndex.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|FeatureIDTexture")
  static int64 GetFeatureIDForTextureCoordinates(
      UPARAM(ref) const FCesiumFeatureIdTexture& FeatureIDTexture,
      float u,
      float v);

  /**
   * Gets the feature ID associated with the given vertex. The
   * feature ID can be used with a FCesiumFeatureTable to retrieve the
   * per-vertex metadata.
   *
   * This works if the vertex contains texture coordinates for the relevant
   * texture coordinate set as indicated by GetTextureCoordinateIndex. If the
   * vertex has no such coordinates, this returns -1.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|FeatureIDTexture")
  static int64 GetFeatureIDForVertex(
      UPARAM(ref) const FCesiumFeatureIdTexture& FeatureIDTexture,
      int64 VertexIndex);
};
