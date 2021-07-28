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
   * Gets all the feature tables that are associated with the primitive.
   *
   * @return All the feature tables that are associated with the primitive
   */
  const TArray<FCesiumMetadataFeatureTable>& GetFeatureTables() const;

  /**
   * Gets the ID of the first vertex that makes up a given face of this
   * primitive.
   *
   * @param faceID The ID of the face.
   */
  int64 GetFirstVertexIDFromFaceID(int64 faceID) const;

private:
  TArray<FCesiumMetadataFeatureTable> _featureTables;
  VertexIDAccessorType _vertexIDAccessor;
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
                       const FCesiumMetadataPrimitive& MetadataPrimitive);

  /**
   * Gets the ID of the first vertex that makes up a given face of this
   * primitive.
   *
   * @param faceID The ID of the face.
   */
  static int64 GetFirstVertexIDFromFaceID(
      UPARAM(ref) const FCesiumMetadataPrimitive& MetadataPrimitive,
      int64 FaceID);
};
