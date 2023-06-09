// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/PropertyTextureView.h"
#include "CesiumPropertyTextureProperty.h"
#include "Containers/Array.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CesiumPropertyTexture.generated.h"

namespace CesiumGltf {
struct Model;
struct ExtensionExtStructuralMetadataPropertyTexture;
}; // namespace CesiumGltf

enum class ECesiumPropertyTextureStatus : uint8{
Valid = 0,
ErrorInvalidMetadataExtension,
ErrorInvalidPropertyTextureClass
}

/**
 * @brief A blueprint-accessible wrapper of a property texture from a glTF.
 * Provides access to {@link FCesiumPropertyTextureProperty} views of texture
 * metadata.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumPropertyTexture {
  GENERATED_USTRUCT_BODY()

public:
  FCesiumPropertyTexture() : _status(ECesiumPropertyTextureStatus::ErrorInvalidMetadataExtension){}

  FCesiumPropertyTexture(
      const CesiumGltf::Model& model,
      const CesiumGltf::ExtensionExtStructuralMetadataPropertyTexture&
          PropertyTexture);

  const CesiumGltf::PropertyTextureView&
  getPropertyTextureView() const {
    return this->_propertyTextureView;
  }

private:
  ECesiumPropertyTextureStatus _status;
  CesiumGltf::PropertyTextureView _propertyTextureView;
  TArray<> _propertyKeys;

  friend class UCesiumPropertyTextureBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumPropertyTextureBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Gets the status of the property texture. If 
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTexture")
  static const ECesiumPropertyTextureStatus
  GetPropertyTextureStatus(UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture);

  /**
   * Gets the FCesiumPropertyTextureProperty views for the given property
   * texture. If this property texture is invalid, this returns an empty array.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTexture")
  static const TArray<FCesiumPropertyTextureProperty>&
  GetProperties(UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture);

  /**
   * Gets the names of the properties in this property texture.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTexture")
  static const TArray<FString>&
  GetPropertyNames(UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture);

  /**
   * Retrieve a FCesiumPropertyTextureProperty by name.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTexture")
  static FCesiumPropertyTextureProperty FindProperty(
      UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture,
      const FString& PropertyName);
};
