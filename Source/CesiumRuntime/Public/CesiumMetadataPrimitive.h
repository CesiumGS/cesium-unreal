// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumMetadataFeatureTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include "CesiumMetadataPrimitive.generated.h"

namespace CesiumGltf {
struct ModelEXT_feature_metadata;
struct MeshPrimitiveEXT_feature_metadata;
} // namespace CesiumGltf

/**
 * A Blueprint-accessible wrapper for a glTF Primitive's Metadata. It holds a
 * collection of feature tables.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataPrimitive {
  GENERATED_USTRUCT_BODY()

public:
  /**
   * Construct an empty primitive metadata.
   */
  FCesiumMetadataPrimitive() {}

  /**
   * Construct a primitive metadata.
   *
   * @param model The model that stores EXT_feature_metadata extension
   * @param primitive The mesh primitive that stores EXT_feature_metadata
   * extension
   * @param metadata The EXT_feature_metadata of the whole gltf
   * @param primitiveMetadata The EXT_feature_metadata of the gltf mesh
   * primitive
   */
  FCesiumMetadataPrimitive(
      const CesiumGltf::Model& model,
      const CesiumGltf::MeshPrimitive& primitive,
      const CesiumGltf::ModelEXT_feature_metadata& metadata,
      const CesiumGltf::MeshPrimitiveEXT_feature_metadata& primitiveMetadata);

  /**
   * Get all the feature tables that are associated with the primitive.
   *
   * @return All the feature tables that are associated with the primitive
   */
  const TArray<FCesiumMetadataFeatureTable>& GetFeatureTables() const;

private:
  TArray<FCesiumMetadataFeatureTable> _featureTables;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataPrimitiveBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Get all the feature tables that are associated with the primitive.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static const TArray<FCesiumMetadataFeatureTable>&
  GetFeatureTables(UPARAM(ref)
                       const FCesiumMetadataPrimitive& metadataPrimitive);
};
