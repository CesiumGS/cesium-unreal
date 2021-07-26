#pragma once

#include "CesiumMetadataFeatureTable.h"
#include "CesiumMetadataGenericValue.h"
#include "CesiumMetadataPrimitive.h"
#include "Containers/UnrealString.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include "CesiumMetadataUtilityBlueprintLibrary.generated.h"

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataUtilityBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Get the primitive metadata of a Gltf primitive component. If component is
   * not a Cesium Gltf primitive component, the returned metadata is empty
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Utility")
  static FCesiumMetadataPrimitive
  GetPrimitiveMetadata(const UPrimitiveComponent* component);

  /**
   * Get the metadata of a face of a gltf primitive component. If the component
   * is not a Cesium Gltf primitive component, the returned metadata is empty
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Utility")
  static TMap<FString, FCesiumMetadataGenericValue>
  GetMetadataValuesForFace(const UPrimitiveComponent* component, int64 faceID);

  /**
   * Get the metadata as string of a face of a gltf primitive component. If the
   * component is not a Cesium Gltf primitive component, the returned metadata
   * is empty
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Utility")
  static TMap<FString, FString> GetMetadataValuesAsStringForFace(
      const UPrimitiveComponent* component,
      int64 faceID);
};
