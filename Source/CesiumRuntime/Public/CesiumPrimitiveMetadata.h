// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"

#include "CesiumPrimitiveMetadata.generated.h"

namespace CesiumGltf {
struct Model;
struct MeshPrimitive;
struct ExtensionModelExtStructuralMetadata;
struct ExtensionMeshPrimitiveExtStructuralMetadata;
} // namespace CesiumGltf

/**
 * A Blueprint-accessible wrapper for a glTF Primitive's EXT_structural_metadata
 * extension. It holds the indices of the property textures / attributes
 * associated with this primitive, which index into the respective arrays in the
 * model's EXT_structural_metadata extension.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumPrimitiveMetadata {
  GENERATED_USTRUCT_BODY()

public:
  /**
   * Construct an empty primitive metadata.
   */
  FCesiumPrimitiveMetadata() {}

  /**
   * Constructs a primitive metadata instance.
   *
   * @param Primitive The mesh primitive containing the EXT_structural_metadata
   * extension
   * @param Metadata The EXT_structural_metadata of the glTF mesh primitive.
   */
  FCesiumPrimitiveMetadata(
      const CesiumGltf::MeshPrimitive& Primitive,
      const CesiumGltf::ExtensionMeshPrimitiveExtStructuralMetadata& Metadata);

private:
  TArray<int64> _propertyTextureIndices;
  TArray<int64> _propertyAttributeIndices;

  friend class UCesiumPrimitiveMetadataBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumPrimitiveMetadataBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Gets the primitive metadata of a glTF primitive component. If component is
   * not a Cesium glTF primitive component, the returned metadata is empty.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|Metadata")
  static const FCesiumPrimitiveMetadata&
  GetPrimitiveMetadata(const UPrimitiveComponent* component);

  /**
   * Get the indices of the property textures that are associated with the
   * primitive. This can be used to retrieve the actual property textures from
   * the model's FCesiumModelMetadata.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|Metadata")
  static const TArray<int64>& GetPropertyTextureIndices(
      UPARAM(ref) const FCesiumPrimitiveMetadata& PrimitiveMetadata);

  /**
   * Get the indices of the property attributes that are associated with the
   * primitive. This can be used to retrieve the actual property attributes from
   * the model's FCesiumModelMetadata.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|Metadata")
  static const TArray<int64>& GetPropertyAttributeIndices(
      UPARAM(ref) const FCesiumPrimitiveMetadata& PrimitiveMetadata);
};
