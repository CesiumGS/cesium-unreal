#pragma once

#include "CesiumFeatureTextureProperty.h"
#include "CesiumGltf/FeatureTextureView.h"
#include "Containers/Map.h"
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

private:
  CesiumGltf::FeatureTextureView _featureTextureView;
  TMap<FString, FCesiumFeatureTextureProperty> _properties;

  friend class UCesiumFeatureTextureBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumFeatureTextureBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTexture")
  static const TMap<FString, FCesiumFeatureTextureProperty>&
  GetProperties(UPARAM(ref) const FCesiumFeatureTexture& featureTexture);
};
