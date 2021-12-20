#pragma once

#include "CesiumFeatureTextureProperty.h"
#include "CesiumGltf/FeatureTextureView.h"
#include "Containers/Array.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CesiumFeatureTexture.generated.h"

namespace CesiumGltf {
struct Model;
struct FeatureTexture;
}; // namespace CesiumGltf

USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumFeatureTexture {
  GENERATED_USTRUCT_BODY()

public:
  FCesiumFeatureTexture() {}

  FCesiumFeatureTexture(
      const CesiumGltf::Model& model,
      const CesiumGltf::FeatureTexture& featureTexture);

  const CesiumGltf::FeatureTextureView& getFeatureTextureView() const {
    return this->_featureTextureView;
  }

private:
  CesiumGltf::FeatureTextureView _featureTextureView;
  TArray<FString> _propertyKeys;

  friend class UCesiumFeatureTextureBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumFeatureTextureBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTexture")
  static const TArray<FString>&
  GetPropertyKeys(UPARAM(ref) const FCesiumFeatureTexture& FeatureTexture);

  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTexture")
  static FCesiumFeatureTextureProperty FindProperty(
      UPARAM(ref) const FCesiumFeatureTexture& FeatureTexture,
      const FString& PropertyName);
};
