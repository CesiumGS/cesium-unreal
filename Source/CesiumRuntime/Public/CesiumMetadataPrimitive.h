// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeatureIdAttribute.h"
#include "CesiumFeatureIdTexture.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
// REMOVE AFTER DEPRECATION
#include "CesiumMetadataFeatureTable.h"

#include "CesiumMetadataPrimitive.generated.h"

// REMOVE AFTER DEPRECATION
struct FCesiumMetadataFeatureTable;

namespace CesiumGltf {
struct ExtensionMeshPrimitiveExtFeatureMetadata;

// REMOVE AFTER DEPRECATION
struct ExtensionModelExtFeatureMetadata;
} // namespace CesiumGltf

/**
 * A Blueprint-accessible wrapper for a glTF Primitive's Metadata. It holds
 * views of feature id attributes / feature id textures and names of feature
 * textures associated with this primitive.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataPrimitive {
  GENERATED_USTRUCT_BODY()

  using VertexIDAccessorType = std::variant<
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint8_t>>,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint16_t>>,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint32_t>>>;

public:
  /**
   * Construct an empty primitive metadata.
   */
  FCesiumMetadataPrimitive() {}

  /**
   * Constructs a primitive metadata instance.
   *
   * @param model The model that stores EXT_feature_metadata extension
   * @param primitive The mesh primitive that stores EXT_feature_metadata
   * extension
   * @param metadata The EXT_feature_metadata of the gltf mesh primitive.
   * primitive
   */
  FCesiumMetadataPrimitive(
      const CesiumGltf::Model& model,
      const CesiumGltf::MeshPrimitive& primitive,
      const CesiumGltf::ExtensionMeshPrimitiveExtFeatureMetadata& metadata,
      const CesiumGltf::ExtensionModelExtFeatureMetadata& modelMetadata);

private:
  TArray<FCesiumFeatureIdAttribute> _featureIdAttributes;
  TArray<FCesiumFeatureIdTexture> _featureIdTextures;
  TArray<FString> _featureTextureNames;
  VertexIDAccessorType _vertexIDAccessor;

  // REMOVE AFTER DEPRECATION
  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  TArray<FCesiumMetadataFeatureTable> _featureTables_deprecated;
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  friend class UCesiumMetadataPrimitiveBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataPrimitiveBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  /**
   * Get all the feature tables that are associated with the primitive.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property",
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "Use UCesiumMetadataModelBlueprintLibrary::GetFeatureTables() instead."))
  static const TArray<FCesiumMetadataFeatureTable>&
  GetFeatureTables(UPARAM(ref)
                       const FCesiumMetadataPrimitive& MetadataPrimitive);
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  /**
   * @brief Get all the feature id attributes that are associated with the
   * primitive.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Primitive")
  static const TArray<FCesiumFeatureIdAttribute>&
  GetFeatureIdAttributes(UPARAM(ref)
                             const FCesiumMetadataPrimitive& MetadataPrimitive);

  /**
   * @brief Get all the feature id textures that are associated with the
   * primitive.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Primitive")
  static const TArray<FCesiumFeatureIdTexture>&
  GetFeatureIdTextures(UPARAM(ref)
                           const FCesiumMetadataPrimitive& MetadataPrimitive);

  /**
   * @brief Get all the feature textures that are associated with the
   * primitive.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Primitive")
  static const TArray<FString>&
  GetFeatureTextureNames(UPARAM(ref)
                             const FCesiumMetadataPrimitive& MetadataPrimitive);

  /**
   * Gets the ID of the first vertex that makes up a given face of this
   * primitive.
   *
   * @param faceID The ID of the face.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Primitive")
  static int64 GetFirstVertexIDFromFaceID(
      UPARAM(ref) const FCesiumMetadataPrimitive& MetadataPrimitive,
      int64 FaceID);
};
