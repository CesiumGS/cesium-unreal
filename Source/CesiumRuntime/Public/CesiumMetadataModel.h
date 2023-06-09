// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumModelMetadata.h"

#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"

#include "CesiumMetadataModel.generated.h"

struct UE_DEPRECATED(
    5.0,
    "FCesiumMetadataModel is deprecated. Instead, use FCesiumModelMetadata to retrieve metadata from a glTF model.")
    FCesiumMetadataModel;

/**
 * @brief A blueprint-accessible wrapper for metadata contained in a glTF model.
 * Provides access to views of feature tables and feature textures available on
 * the glTF.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataModel {
  GENERATED_USTRUCT_BODY()

public:
  FCesiumMetadataModel() : _pModelMetadata(nullptr) {}

  FCesiumMetadataModel(const FCesiumModelMetadata& ModelMetadata);

private:
  const FCesiumModelMetadata* _pModelMetadata;

  friend class UCesiumMetadataModelBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataModelBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  /**
   * @brief Get all the feature tables for this metadata model mapped by name.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Model",
      Meta =
          (DeprecatedFunction,
           DeprecatedMessage =
               "UCesiumMetadataModelBlueprintLibrary is deprecated. Use UCesiumModelMetadataBlueprintLibrary to get FCesiumPropertyTables from FCesiumModelMetadata instead."))
  static const TMap<FString, FCesiumPropertyTable&>
  GetFeatureTables(UPARAM(ref) const FCesiumMetadataModel& MetadataModel);

  /**
   * @brief Get all the feature textures for this metadata model mapped by name.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Model",
      Meta =
          (DeprecatedFunction,
           DeprecatedMessage =
               "UCesiumMetadataModelBlueprintLibrary is deprecated. Use UCesiumModelMetadataBlueprintLibrary to get FCesiumPropertyTextures from FCesiumModelMetadata instead."))
  static const TMap<FString, FCesiumPropertyTexture&>
  GetFeatureTextures(UPARAM(ref) const FCesiumMetadataModel& MetadataModel);
  PRAGMA_ENABLE_DEPRECATION_WARNINGS
};
