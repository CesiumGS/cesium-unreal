// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/PropertyTexturePropertyView.h"
#include "GenericPlatform/GenericPlatform.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CesiumPropertyTextureProperty.generated.h"

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
 * @brief A float color retrieved from a
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

UENUM(BlueprintType)
enum class ECesiumPropertyTexturePropertyStatus : uint8 {
  Valid = 0,

};

/**
 * @brief A blueprint-accessible wrapper for a property texture property from a
 * glTF. Provides per-pixel access to metadata encoded as pixel colors.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumPropertyTextureProperty {
  GENERATED_USTRUCT_BODY()

public:
  FCesiumPropertyTextureProperty() {}

  /*FCesiumPropertyTextureProperty(
      const CesiumGltf::PropertyTexturePropertyView& Property)
      : _pPropertyView(&Property) {}*/

private:
  //  const CesiumGltf::PropertyTexturePropertyView* _pPropertyView;

  friend class UCesiumPropertyTexturePropertyBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumPropertyTexturePropertyBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * @brief Get the index of the texture coordinate set that corresponds to the
   * property texture property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static int64 GetTextureCoordinateIndex(
      const UPrimitiveComponent* PrimitiveComponent,
      UPARAM(ref) const FCesiumPropertyTextureProperty& Property);

  /**
   * @brief Get the component count of this property. Since the metadata is
   * encoded as pixel color, this is also the number of meaningful channels
   * it will use.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static int64
  GetComponentCount(UPARAM(ref) const FCesiumPropertyTextureProperty& Property);

  ///**
  // * Get the string representing how the metadata is encoded in a pixel color.
  // * This is useful to unpack the metadata in the correct order of components
  // * from the pixel color.
  // */
  // UFUNCTION(
  //    BlueprintCallable,
  //    BlueprintPure,
  //    Category = "Cesium|Metadata|PropertyTextureProperty")
  // static FString GetSwizzle(UPARAM(ref)
  //                              const FCesiumPropertyTextureProperty&
  //                              Property);

  ///**
  // * Whether the metadata components are to be interpreted as
  // * normalized values. This only applies when the metadata components have an
  // * integer type.
  // */
  // UFUNCTION(
  //    BlueprintCallable,
  //    BlueprintPure,
  //    Category = "Cesium|Metadata|PropertyTextureProperty")
  // static bool IsNormalized(UPARAM(ref)
  //                             const FCesiumPropertyTextureProperty&
  //                             Property);

  ///**
  // * Given texture coordinates from the appropriate texture coordinate
  // * set (as indicated by GetTextureCoordinateIndex), returns an integer-based
  // * metadata value for the pixel. This automatically unswizzles the pixel
  // * color as needed. Only the first GetComponentCount channels are to be
  // used.
  // */
  // UFUNCTION(
  //    BlueprintCallable,
  //    BlueprintPure,
  //    Category = "Cesium|Metadata|PropertyTextureProperty")
  // static FCesiumIntegerColor GetIntegerColorFromTextureCoordinates(
  //    UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
  //    float u,
  //    float v);

  ///**
  // * Given texture coordinates from the appropriate texture coordinate
  // * set (as indicated by GetTextureCoordinateIndex), returns an float-based
  // * metadata value for the pixel. This automatically unswizzles the pixel
  // * color as needed and normalizes it if needed. Only the first
  // * GetComponentCount channels are to be used.
  // */
  // UFUNCTION(
  //    BlueprintCallable,
  //    BlueprintPure,
  //    Category = "Cesium|Metadata|FCesiumPropertyTextureProperty")
  // static FCesiumFloatColor GetFloatColorFromTextureCoordinates(
  //    UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
  //    float u,
  //    float v);
};
