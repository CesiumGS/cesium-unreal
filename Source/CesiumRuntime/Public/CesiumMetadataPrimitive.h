// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumPrimitiveFeatures.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"

#include "CesiumMetadataPrimitive.generated.h"

namespace CesiumGltf {
struct ExtensionMeshPrimitiveExtFeatureMetadata;
} // namespace CesiumGltf

struct UE_DEPRECATED(
    5.0,
    "FCesiumMetadataPrimitive is deprecated. Use FCesiumPrimitiveFeatures instead to retrieve feature IDs from a glTF primitive.")
    FCesiumMetadataPrimitive;

/**
 * A Blueprint-accessible wrapper for a glTF Primitive's EXT_feature_metadata
 * extension. It has views of feature ID attributes / feature ID textures and
 * names of feature textures associated with this primitive.
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
  FCesiumMetadataPrimitive() : _pPrimitiveFeatures(nullptr) {}

  /**
   * Constructs a primitive metadata instance.
   *
   * This class exists for backwards compatibility, so it requires a
   * FCesiumPrimitiveFeatures to have been constructed beforehand. It assumes
   * the given FCesiumPrimitiveFeatures will have the same lifetime as this
   * instance.
   *
   * @param model The model that stores EXT_feature_metadata extension
   * @param primitive The mesh primitive that stores EXT_feature_metadata
   * extension
   * @param metadata The EXT_feature_metadata of the gltf mesh primitive.
   * primitive
   * @param primitiveFeatures The FCesiumPrimitiveFeatures denoting the feature
   * IDs in the glTF mesh primitive.
   */
  FCesiumMetadataPrimitive(
      const CesiumGltf::Model& model,
      const CesiumGltf::MeshPrimitive& primitive,
      const CesiumGltf::ExtensionMeshPrimitiveExtFeatureMetadata& metadata,
      const FCesiumPrimitiveFeatures& primitiveFeatures);

private:
  const FCesiumPrimitiveFeatures* _pPrimitiveFeatures;
  TArray<FString> _featureTextureNames;

  friend class UCesiumMetadataPrimitiveBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataPrimitiveBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  /**
   * Get all the feature ID attributes that are associated with the
   * primitive.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Primitive",
      Meta =
          (DeprecatedFunction,
           DeprecatedMessage =
               "UCesiumMetadataPrimitiveBlueprintLibrary is deprecated, use UCesiumPrimitiveFeaturesBlueprintLibrary to get FCesiumFeatureIdSet from FCesiumPrimitiveFeatures instead."))
  static const TArray<FCesiumFeatureIdAttribute>
  GetFeatureIdAttributes(UPARAM(ref)
                             const FCesiumMetadataPrimitive& MetadataPrimitive);

  /**
   * Get all the feature ID textures that are associated with the
   * primitive.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Primitive",
      Meta =
          (DeprecatedFunction,
           DeprecatedMessage =
               "UCesiumMetadataPrimitiveBlueprintLibrary is deprecated, use UCesiumPrimitiveFeaturesBlueprintLibrary to get FCesiumFeatureIdSet from FCesiumPrimitiveFeatures instead."))
  static const TArray<FCesiumFeatureIdTexture>
  GetFeatureIdTextures(UPARAM(ref)
                           const FCesiumMetadataPrimitive& MetadataPrimitive);

  /**
   * @brief Get all the feature textures that are associated with the
   * primitive.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Primitive",
      Meta =
          (DeprecatedFunction,
           DeprecatedMessage =
               "UCesiumMetadataPrimitiveBlueprintLibrary is deprecated."))
  // TODO: properly deprecate this function when metadata is overhauled
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
      Category = "Cesium|Metadata|Primitive",
      Meta =
          (DeprecatedFunction,
           DeprecatedMessage =
               "UCesiumMetadataPrimitiveBlueprintLibrary is deprecated. Use UCesiumPrimitiveFeaturesBlueprintLibrary.GetFirstVertexFromFace with FCesiumPrimitiveFeatures instead."))
  static int64 GetFirstVertexIDFromFaceID(
      UPARAM(ref) const FCesiumMetadataPrimitive& MetadataPrimitive,
      int64 FaceID);

  PRAGMA_ENABLE_DEPRECATION_WARNINGS
};
