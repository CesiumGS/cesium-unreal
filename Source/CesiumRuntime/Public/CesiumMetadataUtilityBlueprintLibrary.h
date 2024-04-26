// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeatureIdAttribute.h"
#include "CesiumMetadataPrimitive.h"
#include "CesiumMetadataValue.h"
#include "CesiumPrimitiveMetadata.h"
#include "Containers/UnrealString.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include "CesiumMetadataUtilityBlueprintLibrary.generated.h"

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataUtilityBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  /**
   * Gets the primitive metadata of a glTF primitive component. If component is
   * not a Cesium glTF primitive component, the returned metadata is empty
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "CesiumMetadataPrimitive is deprecated. Use CesiumPrimitiveFeatures and CesiumPrimitiveMetadata instead."))
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
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "The CesiumMetadataUtility blueprint library is deprecated. Use GetMetadataValuesForFace in the CesiumMetadataPicking blueprint library instead."))
  static TMap<FString, FCesiumMetadataValue>
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
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "The CesiumMetadataUtility blueprint library is deprecated. Use GetMetadataValuesForFaceAsStrings in the CesiumMetadataPicking blueprint library instead."))
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
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "Use GetFeatureIDFromFace with CesiumPrimitiveFeatures instead."))
  static int64 GetFeatureIDFromFaceID(
      UPARAM(ref) const FCesiumMetadataPrimitive& Primitive,
      UPARAM(ref) const FCesiumFeatureIdAttribute& FeatureIDAttribute,
      int64 FaceID);
  PRAGMA_ENABLE_DEPRECATION_WARNINGS
};
