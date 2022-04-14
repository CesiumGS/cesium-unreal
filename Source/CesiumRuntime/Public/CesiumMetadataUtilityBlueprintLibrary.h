// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeatureTable.h"
#include "CesiumMetadataGenericValue.h"
#include "CesiumMetadataModel.h"
#include "CesiumMetadataPrimitive.h"
#include "Containers/UnrealString.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include "CesiumMetadataUtilityBlueprintLibrary.generated.h"

struct FCesiumFeatureIdAttribute;
struct FCesiumFeatureIdTexture;

// REMOVE AFTER DEPRECATION
struct FCesiumMetadataFeatureTable;

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataUtilityBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Gets the model metadata of a glTF primitive component. If component is
   * not a Cesium glTF primitive component, the returned metadata is empty
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Utility")
  static const FCesiumMetadataModel&
  GetModelMetadata(const UPrimitiveComponent* component);

  /**
   * Gets the primitive metadata of a glTF primitive component. If component is
   * not a Cesium glTF primitive component, the returned metadata is empty
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Utility")
  static const FCesiumMetadataPrimitive&
  GetPrimitiveMetadata(const UPrimitiveComponent* component);

  /**
   * Gets the metadata of a face of a glTF primitive component. If the component
   * is not a Cesium glTF primitive component, the returned metadata is empty.
   * If the primitive has multiple feature tables, the metadata in the first
   * table is returned.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Utility")
  static TMap<FString, FCesiumMetadataGenericValue>
  GetMetadataValuesForFace(const UPrimitiveComponent* component, int64 faceID);

  /**
   * Gets the metadata as string of a face of a glTF primitive component. If the
   * component is not a Cesium glTF primitive component, the returned metadata
   * is empty. If the primitive has multiple feature tables, the metadata in the
   * first table is returned.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Utility")
  static TMap<FString, FString> GetMetadataValuesAsStringForFace(
      const UPrimitiveComponent* component,
      int64 faceID);

  /**
   * Gets the feature ID associated with a given face for a feature id
   * attribute.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Utility")
  static int64 GetFeatureIDFromFaceID(
      UPARAM(ref) const FCesiumMetadataPrimitive& Primitive,
      UPARAM(ref) const FCesiumFeatureIdAttribute& FeatureIdAttribute,
      int64 faceID);

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  /**
   * Gets the feature ID associated with a given face for a given feature table.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Utility",
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "GetFeatureIDForFace(Primitive, FeatureTable) is deprecated. Please use GetFeatureIDFromFaceID(Primitive, FeatureIdAttribute)."))
  static int64 GetFeatureIDForFace(
      UPARAM(ref) const FCesiumMetadataPrimitive& Primitive,
      UPARAM(ref) const FCesiumMetadataFeatureTable& FeatureTable,
      int64 faceID);
  PRAGMA_ENABLE_DEPRECATION_WARNINGS
};
