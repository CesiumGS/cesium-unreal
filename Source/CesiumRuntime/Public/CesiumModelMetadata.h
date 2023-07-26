// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumPropertyTable.h"
#include "CesiumPropertyTexture.h"

#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"

#include "CesiumModelMetadata.generated.h"

namespace CesiumGltf {
struct ExtensionModelExtStructuralMetadata;
struct Model;
} // namespace CesiumGltf

/**
 * @brief A blueprint-accessible wrapper for metadata contained in a glTF model.
 * Provides access to views of property tables, property textures, and property
 * attributes available on the glTF.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumModelMetadata {
  GENERATED_USTRUCT_BODY()

public:
  FCesiumModelMetadata() {}

  FCesiumModelMetadata(
      const CesiumGltf::Model& InModel,
      const CesiumGltf::ExtensionModelExtStructuralMetadata& Metadata);

private:
  TArray<FCesiumPropertyTable> _propertyTables;
  TArray<FCesiumPropertyTexture> _propertyTextures;
  // TODO: property attributes

  friend class UCesiumModelMetadataBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumModelMetadataBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Gets the model metadata of a glTF primitive component. If component is
   * not a Cesium glTF primitive component, the returned metadata is empty.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Model|Metadata")
  static const FCesiumModelMetadata&
  GetModelMetadata(const UPrimitiveComponent* component);

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  /**
   * @brief Get all the feature tables for this model metadata.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Model",
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "Use GetPropertyTables to get an array of property tables instead."))
  static const TMap<FString, FCesiumPropertyTable>
  GetFeatureTables(UPARAM(ref) const FCesiumModelMetadata& ModelMetadata);

  /**
   * @brief Get all the feature textures for this model metadata.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Model",
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "Use GetPropertyTextures to get an array of property textures instead."))
  static const TMap<FString, FCesiumPropertyTexture>
  GetFeatureTextures(UPARAM(ref) const FCesiumModelMetadata& ModelMetadata);

  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  /**
   * Gets an array of all the property tables for this model metadata.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Model|Metadata")
  static const TArray<FCesiumPropertyTable>&
  GetPropertyTables(UPARAM(ref) const FCesiumModelMetadata& ModelMetadata);

  /**
   * Gets the property table at the specified index for this model metadata. If
   * the index is out-of-bounds, this returns an invalid property table.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Model|Metadata")
  static const FCesiumPropertyTable& GetPropertyTable(
      UPARAM(ref) const FCesiumModelMetadata& ModelMetadata,
      const int64 Index);

  /**
   * Gets the property table at the specified indices for this model metadata.
   * An invalid property table will be returned for any out-of-bounds index.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Model|Metadata")
  static const TArray<FCesiumPropertyTable> GetPropertyTablesAtIndices(
      UPARAM(ref) const FCesiumModelMetadata& ModelMetadata,
      const TArray<int64>& Indices);

  /**
   * Gets an array of all the property textures for this model metadata.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Model|Metadata")
  static const TArray<FCesiumPropertyTexture>&
  GetPropertyTextures(UPARAM(ref) const FCesiumModelMetadata& ModelMetadata);

  /**
   * Gets the property table at the specified index for this model metadata. If
   * the index is out-of-bounds, this returns an invalid property table.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Model|Metadata")
  static const FCesiumPropertyTexture& GetPropertyTexture(
      UPARAM(ref) const FCesiumModelMetadata& ModelMetadata,
      const int64 Index);

  /**
   * Gets an array of the property textures at the specified indices for this
   * model metadata.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Model|Metadata")
  static const TArray<FCesiumPropertyTexture> GetPropertyTexturesAtIndices(
      UPARAM(ref) const FCesiumModelMetadata& ModelMetadata,
      const TArray<int64>& Indices);
};
