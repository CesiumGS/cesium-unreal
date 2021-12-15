#pragma once

#include "CesiumGltf/FeatureTexturePropertyView.h"
#include "GenericPlatform/GenericPlatform.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CesiumFeatureTextureProperty.generated.h"

USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumIntegerColor {
  GENERATED_USTRUCT_BODY()

public:
  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int32 r;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int32 g;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int32 b;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int32 a;
};

USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumFloatColor {
  GENERATED_USTRUCT_BODY()

public:
  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  float r;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  float g;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  float b;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  float a;
};

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

  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTextureProperty")
  static int64
  GetTextureCoordinateIndex(UPARAM(ref)
                                const FCesiumFeatureTextureProperty& property);

  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTextureProperty")
  static int64
  GetComponentCount(UPARAM(ref) const FCesiumFeatureTextureProperty& property);

  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTextureProperty")
  static FCesiumIntegerColor GetIntegerColorFromTextureCoordinates(
      UPARAM(ref) const FCesiumFeatureTextureProperty& property,
      float u,
      float v);

  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTextureProperty")
  static FCesiumFloatColor GetFloatColorFromTextureCoordinates(
      UPARAM(ref) const FCesiumFeatureTextureProperty& property,
      float u,
      float v);
};
