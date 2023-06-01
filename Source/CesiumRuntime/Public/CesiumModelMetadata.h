// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumPropertyTable.h"
//#include "CesiumPropertyTexture.h"

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
 * Provides access to views of property tables and property textures available on
 * the glTF.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumModelMetadata {
  GENERATED_USTRUCT_BODY()

public:
  FCesiumModelMetadata() {}

  FCesiumModelMetadata(
      const CesiumGltf::Model& model,
      const CesiumGltf::ExtensionModelExtStructuralMetadata& metadata);

private:
  TArray<FCesiumPropertyTable> _propertyTables;
  //TMap<FString, FCesiumPropertyTexture> _propertyTextures;

  friend class UCesiumModelMetadataBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumModelMetadataBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * @brief Get all the property tables for this model metadata.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Model")
  static const TArray<FCesiumPropertyTable>&
  GetPropertyTables(UPARAM(ref) const FCesiumModelMetadata& ModelMetadata);

  ///**
  // * @brief Get all the property textures for this model metadata.
  // */
  //UFUNCTION(
  //    BlueprintCallable,
  //    BlueprintPure,
  //    Category = "Cesium|Metadata|Model")
  //static const TMap<FString, FCesiumFeatureTexture>&
  //GetFeatureTextures(UPARAM(ref) const FCesiumModelMetadata& ModelMetadata);
};
