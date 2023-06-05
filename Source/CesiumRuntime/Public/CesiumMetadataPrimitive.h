// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeatureIdAttribute.h"
#include "CesiumFeatureIdTexture.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"

#include "CesiumMetadataPrimitive.generated.h"

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
  TArray<FCesiumFeatureIDAttribute> _featureIdAttributes;
  TArray<FCesiumFeatureIDTexture> _featureIdTextures;
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
  /**
   * @brief Get all the feature id attributes that are associated with the
   * primitive.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Primitive")
  static const TArray<FCesiumFeatureIDAttribute>&
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
  static const TArray<FCesiumFeatureIDTexture>&
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
