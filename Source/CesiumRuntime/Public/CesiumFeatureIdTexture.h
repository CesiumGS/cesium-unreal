// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Containers/UnrealString.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include <CesiumGltf/AccessorUtility.h>
#include <CesiumGltf/FeatureIdTextureView.h>

#include "CesiumFeatureIdTexture.generated.h"

namespace CesiumGltf {
struct Model;
struct FeatureIdTexture;
} // namespace CesiumGltf

/**
 * @brief Reports the status of a FCesiumFeatureIdTexture. If the feature ID
 * texture cannot be accessed, this briefly indicates why.
 */
UENUM(BlueprintType)
enum ECesiumFeatureIdTextureStatus {
  /* The feature ID texture is valid. */
  Valid = 0,
  /* The feature ID texture cannot be found in the glTF, or the texture itself
     has errors. */
  ErrorInvalidTexture,
  /* The feature ID texture is being read in an invalid way -- for example,
     trying to read nonexistent image channels. */
  ErrorInvalidTextureAccess,
};

/**
 * @brief A blueprint-accessible wrapper for a feature ID texture from a glTF
 * primitive. Provides access to per-pixel feature IDs, which can be used with
 * the corresponding {@link FCesiumPropertyTable} to access per-pixel metadata.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumFeatureIdTexture {
  GENERATED_USTRUCT_BODY()

public:
  /**
   * @brief Constructs an empty feature ID texture instance. Empty feature ID
   * textures can be constructed while trying to convert a FCesiumFeatureIdSet
   * that is not an texture. In this case, the status reports it is an invalid
   * texture.
   */
  FCesiumFeatureIdTexture()
      : _status(ECesiumFeatureIdTextureStatus::ErrorInvalidTexture) {}

  /**
   * @brief Constructs a feature ID texture instance.
   *
   * @param Model The model.
   * @param Primitive The mesh primitive containing the feature ID texture.
   * @param FeatureIdTexture The texture specified by the FeatureId.
   * @param PropertyTableName The name of the property table this texture
   * corresponds to, if one exists, for backwards compatibility.
   */
  FCesiumFeatureIdTexture(
      const CesiumGltf::Model& Model,
      const CesiumGltf::MeshPrimitive& Primitive,
      const CesiumGltf::FeatureIdTexture& FeatureIdTexture,
      const FString& PropertyTableName);

  /**
   * @brief Gets the underlying view of this feature ID texture.
   */
  constexpr const CesiumGltf::FeatureIdTextureView&
  getFeatureIdTextureView() const {
    return this->_featureIdTextureView;
  }

private:
  ECesiumFeatureIdTextureStatus _status;
  CesiumGltf::FeatureIdTextureView _featureIdTextureView;
  CesiumGltf::TexCoordAccessorType _texCoordAccessor;
  int64 _textureCoordinateSetIndex;

  // For backwards compatibility.
  FString _propertyTableName;

  friend class UCesiumFeatureIdTextureBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumFeatureIdTextureBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  /**
   * Gets the name of the feature table corresponding to this feature ID
   * texture. The name can be used to fetch the appropriate
   * {@link FCesiumPropertyTable} from the FCesiumMetadataModel.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureIdTexture",
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "Use GetPropertyTableIndex on a CesiumFeatureIdSet instead."))
  static const FString&
  GetFeatureTableName(UPARAM(ref)
                          const FCesiumFeatureIdTexture& FeatureIDTexture);
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  /**
   * Gets the status of the feature ID texture. If this texture is
   * invalid in any way, this will briefly indicate why.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Features|FeatureIDTexture")
  static ECesiumFeatureIdTextureStatus GetFeatureIDTextureStatus(
      UPARAM(ref) const FCesiumFeatureIdTexture& FeatureIDTexture);

  /**
   * Gets the glTF texture coordinate set index used by the feature ID texture.
   * This is the index N corresponding to the "TEXCOORD_N" attribute on the glTF
   * primitive that samples this texture.
   *
   * If the texture contains the `KHR_texture_transform` extension, the original
   * texture coordinate set index can be overridden by the one provided by the
   * extension.
   *
   * If the feature ID texture is invalid, this returns -1.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Features|FeatureIDTexture")
  static int64 GetGltfTextureCoordinateSetIndex(
      UPARAM(ref) const FCesiumFeatureIdTexture& FeatureIDTexture);

  /**
   * Gets the UV channel containing the texture coordinate set that is used by
   * the feature ID texture on the given component. This refers to the UV
   * channel it uses on the primitive's static mesh, which is not necessarily
   * equal to value of GetGltfTextureCoordinateSetIndex.
   *
   * This function may be used with FindCollisionUV to get the feature ID from a
   * line trace hit. However, in order for this function to work, the feature ID
   * texture should be listed under the CesiumFeaturesMetadataComponent of the
   * owner Cesium3DTileset. Otherwise, its texture coordinate set may not be
   * included in the Unreal mesh data. To avoid using
   * CesiumFeaturesMetadataComponent, use GetFeatureIDFromHit instead.
   *
   * This returns -1 if the feature ID texture is invalid, or if the specified
   * texture coordinate set is not present in the component's mesh data.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Features|FeatureIDTexture")
  static int64 GetUnrealUVChannel(
      const UPrimitiveComponent* Component,
      UPARAM(ref) const FCesiumFeatureIdTexture& FeatureIDTexture);

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  /**
   * Gets the feature ID corresponding to the pixel specified by the texture
   * coordinates. The feature ID can be used with a FCesiumPropertyTable to
   * retrieve the per-pixel metadata.
   *
   * This assumes the given texture coordinates are from the appropriate
   * texture coordinate set as indicated by GetTextureCoordinateSetIndex. If the
   * feature ID texture is invalid, this returns -1.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Features|FeatureIDTexture")
  static int64 GetFeatureIDForTextureCoordinates(
      UPARAM(ref) const FCesiumFeatureIdTexture& FeatureIDTexture,
      float U,
      float V);
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  /**
   * Gets the feature ID corresponding to the pixel specified by the UV texture
   * coordinates. The feature ID can be used with a FCesiumPropertyTable to
   * retrieve the per-pixel metadata.
   *
   * If the feature ID texture is invalid, this returns -1.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Features|FeatureIDTexture")
  static int64 GetFeatureIDForUV(
      UPARAM(ref) const FCesiumFeatureIdTexture& FeatureIDTexture,
      const FVector2D& UV);

  /**
   * Gets the feature ID associated with the given vertex. The
   * feature ID can be used with a FCesiumPropertyTable to retrieve the
   * per-vertex metadata.
   *
   * This works if the vertex contains texture coordinates for the relevant
   * texture coordinate set as indicated by GetGltfTextureCoordinateSetIndex. If
   * the vertex has no such coordinates, or if the feature ID texture itself is
   * invalid, this returns -1.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Features|FeatureIDTexture")
  static int64 GetFeatureIDForVertex(
      UPARAM(ref) const FCesiumFeatureIdTexture& FeatureIDTexture,
      int64 VertexIndex);

  /**
   * Gets the feature ID from a given line trace hit on the primitive containing
   * this feature ID texture. The feature ID can be used with a
   * FCesiumPropertyTable to retrieve the corresponding metadata.
   *
   * If the feature ID texture is invalid, this returns -1.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Features|FeatureIDTexture")
  static int64 GetFeatureIDFromHit(
      UPARAM(ref) const FCesiumFeatureIdTexture& FeatureIDTexture,
      const FHitResult& Hit);
};
