// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/PropertyTextureView.h"
#include "CesiumPropertyTextureProperty.h"
#include "Containers/Array.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "CesiumPropertyTexture.generated.h"

namespace CesiumGltf {
struct Model;
struct PropertyTexture;
}; // namespace CesiumGltf

UENUM(BlueprintType)
enum class ECesiumPropertyTextureStatus : uint8 {
  /* The property texture is valid. */
  Valid = 0,
  /* The property texture instance was not initialized from an actual glTF
   property texture. */
  ErrorInvalidPropertyTexture,
  /* The property texture's class could be found in the schema of the metadata
   extension. */
  ErrorInvalidPropertyTextureClass
};

/**
 * @brief A blueprint-accessible wrapper of a property texture from a glTF.
 * Provides access to {@link FCesiumPropertyTextureProperty} views of texture
 * metadata.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumPropertyTexture {
  GENERATED_USTRUCT_BODY()

public:
  FCesiumPropertyTexture()
      : _status(ECesiumPropertyTextureStatus::ErrorInvalidPropertyTexture) {}

  FCesiumPropertyTexture(
      const CesiumGltf::Model& model,
      const CesiumGltf::PropertyTexture& PropertyTexture);

  /**
   * Gets the name of the metadata class that this property table conforms to.
   */
  FString getClassName() const { return _className; }

private:
  ECesiumPropertyTextureStatus _status;
  FString _name;
  FString _className;

  TMap<FString, FCesiumPropertyTextureProperty> _properties;

  friend class UCesiumPropertyTextureBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumPropertyTextureBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Gets the status of the property texture. If the property texture is invalid
   * in any way, this briefly indicates why.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTexture")
  static const ECesiumPropertyTextureStatus
  GetPropertyTextureStatus(UPARAM(ref)
                               const FCesiumPropertyTexture& PropertyTexture);

  /**
   * Gets the name of the property texture.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTexture")
  static const FString&
  GetPropertyTextureName(UPARAM(ref)
                             const FCesiumPropertyTexture& PropertyTexture);

  /**
   * Gets all the properties of the property texture, mapped by property name.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTexture")
  static const TMap<FString, FCesiumPropertyTextureProperty>
  GetProperties(UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture);

  /**
   * Gets the names of the properties in this property texture. If the property
   * texture is invalid, this returns an empty array.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTexture")
  static const TArray<FString>
  GetPropertyNames(UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture);

  /**
   * Retrieve a FCesiumPropertyTextureProperty by name. If the property texture
   * does not contain a property with that name, this returns an invalid
   * FCesiumPropertyTextureProperty.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTexture")
  static const FCesiumPropertyTextureProperty& FindProperty(
      UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture,
      const FString& PropertyName);

  /**
   * Gets all of the property values at the given texture coordinates, mapped by
   * property name. This will only include values from valid property texture
   * properties.
   *
   * In EXT_structural_metadata, individual properties can specify different
   * texture coordinate sets to be sampled from. This method uses the same
   * coordinates to sample each property, regardless of its intended texture
   * coordinate set. Use GetMetadataValuesForHit instead to sample the property
   * texture's properties with their respective texture coordinate sets.
   *
   * @param UV The texture coordinates.
   * @return The property values mapped by property name.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTexture")
  static TMap<FString, FCesiumMetadataValue> GetMetadataValuesForUV(
      UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture,
      const FVector2D& UV);

  /**
   * Given a trace hit result, gets all of the property values from property
   * texture on the hit component, mapped by property name. This will only
   * include values from valid property texture properties.
   *
   * In EXT_structural_metadata, individual properties can specify different
   * texture coordinate sets to be sampled from. This method uses the
   * corresponding texture coordinate sets to sample each property.
   *
   * @param Hit The trace hit result
   * @return The property values mapped by property name.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTexture")
  static TMap<FString, FCesiumMetadataValue> GetMetadataValuesFromHit(
      UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture,
      const FHitResult& Hit);
};
