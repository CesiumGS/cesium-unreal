// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumPrimitiveFeatures.h"
#include "CesiumPrimitiveMetadata.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"

#include "CesiumMetadataPrimitive.generated.h"

struct UE_DEPRECATED(
    5.0,
    "FCesiumMetadataPrimitive is deprecated. Instead, use FCesiumPrimitiveFeatures and FCesiumPrimitiveMetadata to retrieve feature IDs and metadata from a glTF primitive.")
    FCesiumMetadataPrimitive;

/**
 * A Blueprint-accessible wrapper for a glTF Primitive's EXT_feature_metadata
 * extension. This class is deprecated and only exists for backwards
 * compatibility.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataPrimitive {
  GENERATED_USTRUCT_BODY()

public:
  /**
   * Construct an empty primitive metadata.
   */
  FCesiumMetadataPrimitive()
      : _pPrimitiveFeatures(nullptr), _pPrimitiveMetadata(nullptr) {}

  /**
   * Constructs a primitive metadata instance from the new features / metadata
   * implementations for backwards compatibility.
   *
   * @param primitiveFeatures The FCesiumPrimitiveFeatures denoting the feature
   * IDs in the glTF mesh primitive.
   * @param primitiveMetadata The FCesiumPrimitiveMetadata containing references
   * to the metadata for the glTF mesh primitive.
   */
  FCesiumMetadataPrimitive(const FCesiumPrimitiveFeatures& primitiveFeatures);

private:
  const FCesiumPrimitiveFeatures* _pPrimitiveFeatures;
  const FCesiumPrimitiveMetadata* _pPrimitiveMetadata;

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
               "UCesiumMetadataPrimitiveBlueprintLibrary is deprecated. Use UCesiumPrimitiveFeaturesBlueprintLibrary to get FCesiumFeatureIdSet from FCesiumPrimitiveFeatures instead."))
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
               "UCesiumMetadataPrimitiveBlueprintLibrary is deprecated. Use UCesiumPrimitiveFeaturesBlueprintLibrary to get FCesiumFeatureIdSet from FCesiumPrimitiveFeatures instead."))
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
               "UCesiumMetadataPrimitiveBlueprintLibrary is deprecated. Use UCesiumPrimitiveMetadataBlueprintLibrary to get property texture indices from FCesiumPrimitiveMetadata instead."))
  static const TArray<FString>
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
