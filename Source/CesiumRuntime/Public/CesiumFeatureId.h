// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeatureIdAttribute.h"
#include "CesiumFeatureIdTexture.h"
#include "CesiumFeatureTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CesiumFeatureId.generated.h"

namespace CesiumGltf {
struct Model;
struct ExtensionExtMeshFeaturesFeatureId;
} // namespace CesiumGltf

UENUM(BlueprintType)
enum ECesiumFeatureIDType { None, Attribute, Texture, Implicit };

/**
 * @brief A blueprint-accessible wrapper for a feature ID from a glTF
 * primitive. A feature ID can be defined as a per-vertex attribute, as a
 * feature texture, or implicitly via vertex ID. These can be used with the
 * corresponding {@link FCesiumFeatureTable} to access per-vertex metadata.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumFeatureID {
  GENERATED_USTRUCT_BODY()

  using FeatureIDType = std::variant<
      std::monostate,
      FCesiumFeatureIDAttribute,
      FCesiumFeatureIDTexture>;

public:
  FCesiumFeatureID() {}

  FCesiumFeatureID(
      const CesiumGltf::Model& Model,
      const CesiumGltf::MeshPrimitive& Primitive,
      const CesiumGltf::ExtensionExtMeshFeaturesFeatureId& FeatureId);

private:
  FeatureIDType _featureID;
  ECesiumFeatureIDType _featureIDType;
  int64 _featureCount;
  TOptional<int64> _propertyTableIndex;

  friend class UCesiumFeatureIDBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumFeatureIDBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()
public:
  /**
   * Gets the type of this feature ID.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|FeatureID")
  static const ECesiumFeatureIDType
  GetFeatureIDType(UPARAM(ref) const FCesiumFeatureID& FeatureID);

  /**
   * Gets this feature ID as a feature ID attribute. This can be used for more
   * fine-grained interaction with the attribute itself. If this feature ID is
   * not defined as an attribute, then the returned attribute will be invalid.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|FeatureID")
  static const FCesiumFeatureIDAttribute
  GetAsFeatureIDAttribute(UPARAM(ref) const FCesiumFeatureID& FeatureID);

  /**
   * Gets this feature ID as a feature ID texture. This can be used for more
   * fine-grained interaction with the texture itself. If this feature ID is
   * not defined as a texture, then the returned texture will be invalid.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|FeatureID")
  static const FCesiumFeatureIDTexture
  GetAsFeatureIDTexture(UPARAM(ref) const FCesiumFeatureID& FeatureID);

  /**
   * Get the index of the property table corresponding to this feature
   * ID. The index can be used to fetch the appropriate
   * FCesiumFeatureTable from the FCesiumMetadataModel. If the
   * feature ID does not specify a property table, this returns -1.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|FeatureID")
  static const int64
  GetPropertyTableIndex(UPARAM(ref) const FCesiumFeatureID& FeatureID);

  /**
   * Get the number of features this primitive has.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|FeatureID")
  static int64 GetFeatureCount(UPARAM(ref) const FCesiumFeatureID& FeatureID);

  /**
   * Gets the feature ID associated with a given vertex. The feature ID can be
   * used with a FCesiumFeatureTable to retrieve the per-vertex
   * metadata.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|FeatureID")
  static int64 GetFeatureIDForVertex(
      UPARAM(ref) const FCesiumFeatureID& FeatureID,
      int64 VertexIndex);
};
