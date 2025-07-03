// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumPropertyAttribute.h"
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
 * extension. It holds the property attributes used by the primitive, as well as
 * the indices of the property textures associated with it, which index into the
 * array of property textures in the model's EXT_structural_metadata extension.
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
   * @param model The model containing the given mesh primitive.
   * @param primitive The mesh primitive containing the EXT_structural_metadata
   * extension
   * @param metadata The EXT_structural_metadata of the glTF mesh primitive.
   */
  FCesiumPrimitiveMetadata(
      const CesiumGltf::Model& model,
      const CesiumGltf::MeshPrimitive& primitive,
      const CesiumGltf::ExtensionMeshPrimitiveExtStructuralMetadata& metadata);

private:
  TArray<int64> _propertyTextureIndices;
  TArray<FCesiumPropertyAttribute> _propertyAttributes;

  // For backwards compatibility with GetPropertyAttributeIndices().
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
   * Get the property attributes that are associated with the primitive.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|Metadata")
  static const TArray<FCesiumPropertyAttribute>&
  GetPropertyAttributes(UPARAM(ref)
                            const FCesiumPrimitiveMetadata& PrimitiveMetadata);

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  /**
   * Get the indices of the property attributes that are associated with the
   * primitive.
   */
  UFUNCTION(
      Category = "Cesium|Primitive|Metadata",
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "Retrieve property attributes directly through GetPropertyAttributes instead."))
  static const TArray<int64>& GetPropertyAttributeIndices(
      UPARAM(ref) const FCesiumPrimitiveMetadata& PrimitiveMetadata);
  PRAGMA_ENABLE_DEPRECATION_WARNINGS
};
