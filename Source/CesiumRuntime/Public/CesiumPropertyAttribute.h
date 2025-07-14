// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumMetadataEnum.h"
#include "CesiumMetadataValue.h"
#include "CesiumPropertyAttributeProperty.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include "CesiumPropertyAttribute.generated.h"

namespace CesiumGltf {
struct Model;
struct PropertyAttribute;
} // namespace CesiumGltf

/**
 * @brief Reports the status of a FCesiumPropertyAttribute. If the property
 * attribute cannot be accessed, this briefly indicates why.
 */
UENUM(BlueprintType)
enum class ECesiumPropertyAttributeStatus : uint8 {
  /* The property attribute is valid. */
  Valid = 0,
  /* The property attribute instance was not initialized from an actual glTF
     property attribute. */
  ErrorInvalidPropertyAttribute,
  /* The property attribute's class could not be found in the schema of the
     metadata extension. */
  ErrorInvalidPropertyAttributeClass
};

/**
 * A Blueprint-accessible wrapper for a glTF property attribute.
 * Provides access to {@link FCesiumPropertyAttributeProperty} views of
 * per-vertex metadata.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumPropertyAttribute {
  GENERATED_USTRUCT_BODY()

public:
  /**
   * Construct an empty property attribute instance.
   */
  FCesiumPropertyAttribute()
      : _status(ECesiumPropertyAttributeStatus::ErrorInvalidPropertyAttribute) {
  }

  /**
   * Constructs a property attribute from the given glTF.
   *
   * @param model The model that stores EXT_structural_metadata.
   * @param primitive The primitive that contains the target property attribute.
   * @param propertyAttribute The target property attribute.
   */
  FCesiumPropertyAttribute(
      const CesiumGltf::Model& model,
      const CesiumGltf::MeshPrimitive& primitive,
      const CesiumGltf::PropertyAttribute& propertyAttribute)
      : FCesiumPropertyAttribute(
            model,
            primitive,
            propertyAttribute,
            FCesiumMetadataEnumCollection::GetOrCreateFromModel(model)) {}

  /**
   * Constructs a property attribute from the given glTF.
   *
   * @param model The model that stores EXT_structural_metadata.
   * @param primitive The primitive that contains the target property attribute.
   * @param propertyAttribute The target property attribute.
   * @param pEnumCollection The enum collection to use, if any.
   */
  FCesiumPropertyAttribute(
      const CesiumGltf::Model& model,
      const CesiumGltf::MeshPrimitive& primitive,
      const CesiumGltf::PropertyAttribute& propertyAttribute,
      const TSharedPtr<FCesiumMetadataEnumCollection>& pEnumCollection);

  /**
   * Gets the name of the metadata class that this property attribute conforms
   * to.
   */
  FString getClassName() const { return _className; }

private:
  ECesiumPropertyAttributeStatus _status;
  FString _name;
  FString _className;

  TMap<FString, FCesiumPropertyAttributeProperty> _properties;

  friend class UCesiumPropertyAttributeBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumPropertyAttributeBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Gets the status of the property attribute. If an error occurred while
   * parsing the property attribute from the glTF extension, this briefly
   * conveys why.
   *
   * @param PropertyAttribute The property attribute.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttribute")
  static ECesiumPropertyAttributeStatus GetPropertyAttributeStatus(
      UPARAM(ref) const FCesiumPropertyAttribute& PropertyAttribute);

  /**
   * Gets the name of the property attribute. If no name was specified in the
   * glTF extension, this returns an empty string.
   *
   * @param PropertyAttribute The property attribute.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttribute")
  static const FString& GetPropertyAttributeName(
      UPARAM(ref) const FCesiumPropertyAttribute& PropertyAttribute);

  /**
   * Gets all the properties of the property attribute, mapped by property name.
   *
   * @param PropertyAttribute The property attribute.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttribute")
  static const TMap<FString, FCesiumPropertyAttributeProperty>&
  GetProperties(UPARAM(ref) const FCesiumPropertyAttribute& PropertyAttribute);

  /**
   * Gets the names of the properties in this property attribute.
   *
   * @param PropertyAttribute The property attribute.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttribute")
  static const TArray<FString>
  GetPropertyNames(UPARAM(ref)
                       const FCesiumPropertyAttribute& PropertyAttribute);

  /**
   * Retrieve a FCesiumPropertyAttributeProperty by name. If the property
   * attribute does not contain a property with that name, this returns an
   * invalid FCesiumPropertyAttributeProperty.
   *
   * @param PropertyAttribute The property attribute.
   * @param PropertyName The name of the property to find.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttribute")
  static const FCesiumPropertyAttributeProperty& FindProperty(
      UPARAM(ref) const FCesiumPropertyAttribute& PropertyAttribute,
      const FString& PropertyName);

  /**
   * Gets all of the property values for the given vertex index, mapped by
   * property name. This will only include values from valid property attribute
   * properties.
   *
   * If the index is out-of-bounds, the returned map will be empty.
   *
   * @param PropertyAttribute The property attribute.
   * @param Index The index of the target vertex.
   * @return The property values mapped by property name.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttribute")
  static TMap<FString, FCesiumMetadataValue> GetMetadataValuesAtIndex(
      UPARAM(ref) const FCesiumPropertyAttribute& PropertyAttribute,
      int64 Index);
};
