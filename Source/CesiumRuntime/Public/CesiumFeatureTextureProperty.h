// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/FeatureTexturePropertyView.h"
#include "GenericPlatform/GenericPlatform.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CesiumFeatureTextureProperty.generated.h"

/**
 * @brief An integer color retrieved from a
 * {@link FCesiumFeatureTextureProperty}.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumIntegerColor {
  GENERATED_USTRUCT_BODY()

public:
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  int32 r = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  int32 g = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  int32 b = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  int32 a = 0;
};

/**
 * @brief An float color retrieved from a
 * {@link FCesiumFeatureTextureProperty}.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumFloatColor {
  GENERATED_USTRUCT_BODY()

public:
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  float r = 0.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  float g = 0.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  float b = 0.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  float a = 0.0f;
};

/**
 * @brief A blueprint-accessible wrapper for a feature texture property from a
 * glTF. Provides per-pixel access to metadata encoded as pixel color.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumFeatureTextureProperty {
  GENERATED_USTRUCT_BODY()

public:
  FCesiumFeatureTextureProperty() {}

  FCesiumFeatureTextureProperty(
      const CesiumGltf::FeatureTexturePropertyView& property)
      : _pProperty(&property) {}

private:
  const CesiumGltf::FeatureTexturePropertyView* _pProperty;

  friend class UCesiumFeatureTexturePropertyBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumFeatureTexturePropertyBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * @brief Get the index of the texture coordinate set that corresponds to the
   * feature texture property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTextureProperty")
  static int64 GetTextureCoordinateIndex(
      const UPrimitiveComponent* component,
      UPARAM(ref) const FCesiumFeatureTextureProperty& property);

  /**
   * @brief Get the component count of this property. Since the metadata is
   * encoded as pixel color, this is also the number of meaningful channels
   * it will use.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTextureProperty")
  static int64
  GetComponentCount(UPARAM(ref) const FCesiumFeatureTextureProperty& property);

  /**
   * @brief Get the string representing how the metadata is encoded into a
   * pixel color. This is useful to unpack the correct order of the metadata
   * components from the pixel color.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTextureProperty")
  static FString GetSwizzle(UPARAM(ref)
                                const FCesiumFeatureTextureProperty& property);

  /**
   * @brief Whether the metadata components are to be interpreted as
   * normalized. This only applies when the metadata components have an integer
   * type.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTextureProperty")
  static bool IsNormalized(UPARAM(ref)
                               const FCesiumFeatureTextureProperty& property);

  /**
   * @brief Given texture coordinates from the appropriate texture coordinate
   * set (as indicated by GetTextureCoordinateIndex), returns an integer-based
   * metadata value for the pixel. This automatically unswizzles the pixel
   * color as needed. Only the first GetComponentCount channels are to be used.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTextureProperty")
  static FCesiumIntegerColor GetIntegerColorFromTextureCoordinates(
      UPARAM(ref) const FCesiumFeatureTextureProperty& property,
      float u,
      float v);

  /**
   * @brief Given texture coordinates from the appropriate texture coordinate
   * set (as indicated by GetTextureCoordinateIndex), returns an float-based
   * metadata value for the pixel. This automatically unswizzles the pixel
   * color as needed and normalizes it if needed. Only the first
   * GetComponentCount channels are to be used.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTextureProperty")
  static FCesiumFloatColor GetFloatColorFromTextureCoordinates(
      UPARAM(ref) const FCesiumFeatureTextureProperty& property,
      float u,
      float v);
};
