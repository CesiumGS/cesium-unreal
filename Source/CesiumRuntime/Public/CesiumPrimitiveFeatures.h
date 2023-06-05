// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeatureId.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"

#include "CesiumPrimitiveFeatures.generated.h"

namespace CesiumGltf {
struct ExtensionExtMeshFeatures;
} // namespace CesiumGltf

/**
 * A Blueprint-accessible wrapper for a glTF Primitive's mesh features. It holds
 * views of the feature ID attributes / textures associated with this
 * primitive. If the primitive has EXT_mesh_features, but no attributes or
 * textures are defined, then the feature IDs are implicit via vertex index.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumPrimitiveFeatures {
  GENERATED_USTRUCT_BODY()

  using VertexIDAccessorType = std::variant<
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint8_t>>,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint16_t>>,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint32_t>>>;

public:
  /**
   * Construct an empty primitive features.
   */
  FCesiumPrimitiveFeatures() {}

  /**
   * Constructs a primitive features instance.
   *
   * @param model The model that contains the EXT_mesh_features extension
   * @param primitive The mesh primitive that stores EXT_feature_metadata
   * extension
   * @param features The EXT_mesh_features of the gltf mesh primitive.
   * primitive
   */
  FCesiumPrimitiveFeatures(
      const CesiumGltf::Model& model,
      const CesiumGltf::MeshPrimitive& primitive,
      const CesiumGltf::ExtensionExtMeshFeatures& features);

private:
  TArray<FCesiumFeatureID> _featureIDs;
  VertexIDAccessorType _vertexIDAccessor;

  friend class UCesiumPrimitiveFeaturesBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumPrimitiveFeaturesBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Gets all the feature IDs that are associated with the
   * primitive.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|Features")
  static const TArray<FCesiumFeatureID>&
  GetFeatureIDs(UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures);

  /**
   * Gets all the feature IDs of the given type. If the primitive has none of that type, the returned array will be empty.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|Features")
  static const TArray<FCesiumFeatureID>
  GetFeatureIDsOfType(UPARAM(ref) const FCesiumPrimitiveFeatures& MetadataPrimitive,
      ECesiumFeatureIDType Type);

  /**
   * Gets the index of the first vertex that makes up a given face of this
   * primitive.
   *
   * @param FaceIndex The index of the face.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|Features")
  static int64 GetFirstVertexIndexFromFaceIndex(
      UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures,
      int64 FaceIndex);

  
  /**
   * Gets the feature ID associated with a given face and feature ID from this primitive.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|Features")
  static int64 GetFeatureIDFromFaceIndex(
      UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures,
      UPARAM(ref) const FCesiumFeatureID& FeatureID,
      int64 FaceIndex);
};
