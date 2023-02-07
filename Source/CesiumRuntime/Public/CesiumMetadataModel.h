// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeatureTable.h"
#include "CesiumFeatureTexture.h"

#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"

#include "CesiumMetadataModel.generated.h"

namespace CesiumGltf {
struct ExtensionModelExtFeatureMetadata;
struct Model;
} // namespace CesiumGltf

/**
 * @brief A blueprint-accessible wrapper for metadata contained in a glTF model.
 * Provides access to views of feature tables and feature textures available on
 * the glTF.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataModel {
  GENERATED_USTRUCT_BODY()

public:
  FCesiumMetadataModel() {}

  FCesiumMetadataModel(
      const CesiumGltf::Model& model,
      const CesiumGltf::ExtensionModelExtFeatureMetadata& metadata);

private:
  TMap<FString, FCesiumFeatureTable> _featureTables;
  TMap<FString, FCesiumFeatureTexture> _featureTextures;

  friend class UCesiumMetadataModelBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataModelBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * @brief Get all the feature tables for this metadata model.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Model")
  static const TMap<FString, FCesiumFeatureTable>&
  GetFeatureTables(UPARAM(ref) const FCesiumMetadataModel& MetadataModel);

  /**
   * @brief Get all the feature textures for this metadata model.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Model")
  static const TMap<FString, FCesiumFeatureTexture>&
  GetFeatureTextures(UPARAM(ref) const FCesiumMetadataModel& MetadataModel);
};
