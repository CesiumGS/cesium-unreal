// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumPropertyTexture.h"
#include "CesiumMetadataPickingBlueprintLibrary.h"

#include <CesiumGltf/Model.h>
#include <CesiumGltf/PropertyTexturePropertyView.h>
#include <CesiumGltf/PropertyTextureView.h>

static FCesiumPropertyTextureProperty EmptyPropertyTextureProperty;

FCesiumPropertyTexture::FCesiumPropertyTexture(
    const CesiumGltf::Model& model,
    const CesiumGltf::PropertyTexture& propertyTexture,
    const TSharedPtr<FCesiumMetadataEnumCollection>& pEnumCollection)
    : _status(ECesiumPropertyTextureStatus::ErrorInvalidPropertyTextureClass),
      _name(propertyTexture.name.value_or("").c_str()),
      _className(propertyTexture.classProperty.c_str()) {
  CesiumGltf::PropertyTextureView propertyTextureView(model, propertyTexture);
  switch (propertyTextureView.status()) {
  case CesiumGltf::PropertyTextureViewStatus::Valid:
    _status = ECesiumPropertyTextureStatus::Valid;
    break;
  default:
    // Status was already set in initializer list.
    return;
  }

  const CesiumGltf::ExtensionModelExtStructuralMetadata* pExtension =
      model.getExtension<CesiumGltf::ExtensionModelExtStructuralMetadata>();
  // If there was no schema, we would've gotten ErrorMissingSchema for the
  // propertyTextureView status.
  check(pExtension != nullptr && pExtension->schema != nullptr);

  const CesiumGltf::Class* pClass = propertyTextureView.getClass();
  for (const auto& classPropertyPair : pClass->properties) {
    {
      const auto& propertyPair =
          propertyTexture.properties.find(classPropertyPair.first);
      if (propertyPair == propertyTexture.properties.end()) {
        continue;
      }

      CesiumGltf::TextureViewOptions options;
      options.applyKhrTextureTransformExtension = true;

      if (propertyPair->second.extras.find("makeImageCopy") !=
          propertyPair->second.extras.end()) {
        options.makeImageCopy = propertyPair->second.extras.at("makeImageCopy")
                                    .getBoolOrDefault(false);
      }

      propertyTextureView.getPropertyView(
          propertyPair->first,
          [&properties = this->_properties,
           &pEnumCollection,
           &propertyTextureView,
           &classProperty = classPropertyPair.second,
           &Schema = *pExtension->schema](
              const std::string& propertyId,
              auto propertyValue) mutable {
            FString key(UTF8_TO_TCHAR(propertyId.data()));

            TSharedPtr<FCesiumMetadataEnum> pEnumDefinition = nullptr;
            if (pEnumCollection.IsValid() &&
                classProperty.enumType.has_value()) {
              pEnumDefinition = pEnumCollection->Get(FString(
                  UTF8_TO_TCHAR(classProperty.enumType.value().c_str())));
            }

            properties.Add(
                key,
                FCesiumPropertyTextureProperty(propertyValue, pEnumDefinition));
          },
          options);
    }
  }
}

/*static*/ const ECesiumPropertyTextureStatus
UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureStatus(
    UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture) {
  return PropertyTexture._status;
}

/*static*/ const FString&
UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureName(
    UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture) {
  return PropertyTexture._name;
}

/*static*/ const TMap<FString, FCesiumPropertyTextureProperty>
UCesiumPropertyTextureBlueprintLibrary::GetProperties(
    UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture) {
  return PropertyTexture._properties;
}

/*static*/ const TArray<FString>
UCesiumPropertyTextureBlueprintLibrary::GetPropertyNames(
    UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture) {
  TArray<FString> names;
  PropertyTexture._properties.GenerateKeyArray(names);
  return names;
}

/*static*/ const FCesiumPropertyTextureProperty&
UCesiumPropertyTextureBlueprintLibrary::FindProperty(
    UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture,
    const FString& PropertyName) {
  const FCesiumPropertyTextureProperty* property =
      PropertyTexture._properties.Find(PropertyName);
  return property ? *property : EmptyPropertyTextureProperty;
}

/*static*/ TMap<FString, FCesiumMetadataValue>
UCesiumPropertyTextureBlueprintLibrary::GetMetadataValuesForUV(
    UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture,
    const FVector2D& UV) {
  TMap<FString, FCesiumMetadataValue> values;

  for (const auto& propertyIt : PropertyTexture._properties) {
    const FCesiumPropertyTextureProperty& property = propertyIt.Value;
    ECesiumPropertyTexturePropertyStatus status =
        UCesiumPropertyTexturePropertyBlueprintLibrary::
            GetPropertyTexturePropertyStatus(property);
    if (status == ECesiumPropertyTexturePropertyStatus::Valid) {
      values.Add(
          propertyIt.Key,
          UCesiumPropertyTexturePropertyBlueprintLibrary::GetValue(
              propertyIt.Value,
              UV));
    } else if (
        status ==
        ECesiumPropertyTexturePropertyStatus::EmptyPropertyWithDefault) {
      values.Add(
          propertyIt.Key,
          UCesiumPropertyTexturePropertyBlueprintLibrary::GetDefaultValue(
              propertyIt.Value));
    }
  }

  return values;
}

/*static*/ TMap<FString, FCesiumMetadataValue>
UCesiumPropertyTextureBlueprintLibrary::GetMetadataValuesFromHit(
    UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture,
    const FHitResult& Hit) {
  TMap<FString, FCesiumMetadataValue> values;

  for (const auto& propertyIt : PropertyTexture._properties) {
    if (UCesiumPropertyTexturePropertyBlueprintLibrary::
            GetPropertyTexturePropertyStatus(propertyIt.Value) ==
        ECesiumPropertyTexturePropertyStatus::EmptyPropertyWithDefault) {
      values.Add(
          propertyIt.Key,
          UCesiumPropertyTexturePropertyBlueprintLibrary::GetDefaultValue(
              propertyIt.Value));
      continue;
    }

    if (UCesiumPropertyTexturePropertyBlueprintLibrary::
            GetPropertyTexturePropertyStatus(propertyIt.Value) !=
        ECesiumPropertyTexturePropertyStatus::Valid) {
      continue;
    }

    auto glTFTexCoordIndex = propertyIt.Value.getTexCoordSetIndex();
    FVector2D UV;
    if (UCesiumMetadataPickingBlueprintLibrary::FindUVFromHit(
            Hit,
            glTFTexCoordIndex,
            UV)) {
      values.Add(
          propertyIt.Key,
          UCesiumPropertyTexturePropertyBlueprintLibrary::GetValue(
              propertyIt.Value,
              UV));
    }
  }
  return values;
}
