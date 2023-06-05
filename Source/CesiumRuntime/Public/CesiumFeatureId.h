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
enum FeatureIDType { None, Attribute, Texture, Implicit };

/**
 * @brief A blueprint-accessible wrapper for a feature ID from a glTF
 * primitive. A feature ID can be defined as a per-vertex attribute, as a
 * feature texture, or implicitly via vertex ID. These can be used with the
 * corresponding {@link FCesiumFeatureTable} to access per-vertex metadata.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumFeatureID {
  GENERATED_USTRUCT_BODY()

  using CesiumFeatureIDType = std::variant<
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
  CesiumFeatureIDType _featureID;
  FeatureIDType _featureIDType;
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
  static const FeatureIDType
  GetFeatureIDType(UPARAM(ref) const FCesiumFeatureID& FeatureID);

  /**
   * Get the index of the property table corresponding to this feature
   * ID. The index can be used to fetch the appropriate
   * {@link FCesiumFeatureTable} from the {@link FCesiumMetadataModel}. If the
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
   * used with a {@link FCesiumFeatureTable} to retrieve the per-vertex
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
