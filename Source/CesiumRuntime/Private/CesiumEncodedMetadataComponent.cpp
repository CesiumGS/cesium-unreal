
#include "CesiumEncodedMetadataComponent.h"
#include "Cesium3DTileset.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumMetadataConversions.h"
#include "CesiumMetadataModel.h"

void UCesiumEncodedMetadataComponent::AutoFill() {
  const ACesium3DTileset* pOwner = this->GetOwner<ACesium3DTileset>();
  if (!pOwner) {
    return;
  }

  for (const UActorComponent* pComponent : pOwner->GetComponents()) {
    const UCesiumGltfComponent* pGltf = Cast<UCesiumGltfComponent>(pComponent);
    if (!pGltf) {
      continue;
    }

    const FCesiumMetadataModel& model = pGltf->Metadata;
    const TMap<FString, FCesiumMetadataFeatureTable>& featureTables =
        UCesiumMetadataModelBlueprintLibrary::GetFeatureTables(model);
    const TMap<FString, FCesiumFeatureTexture>& featureTextures =
        UCesiumMetadataModelBlueprintLibrary::GetFeatureTextures(model);

    for (const auto& featureTableIt : featureTables) {
      const TMap<FString, FCesiumMetadataProperty>& properties =
          UCesiumMetadataFeatureTableBlueprintLibrary::GetProperties(
              featureTableIt.Value);

      FFeatureTableDescription* pFeatureTable = nullptr;
      for (FFeatureTableDescription& existingFeatureTable :
           this->FeatureTables) {
        if (existingFeatureTable.Name == featureTableIt.Key) {
          pFeatureTable = &existingFeatureTable;
          break;
        }
      }

      if (!pFeatureTable) {
        pFeatureTable = &this->FeatureTables.Emplace_GetRef();
        pFeatureTable->Name = featureTableIt.Key;
      }

      for (const auto& propertyIt : properties) {
        bool propertyFound = false;
        for (const FPropertyDescription& existingProperty :
             pFeatureTable->Properties) {
          if (existingProperty.Name == propertyIt.Key) {
            propertyFound = true;
            break;
          }
        }

        if (propertyFound) {
          continue;
        }

        ECesiumMetadataTrueType type =
            UCesiumMetadataPropertyBlueprintLibrary::GetTrueType(
                propertyIt.Value);
        ECesiumMetadataTrueType componentType =
            UCesiumMetadataPropertyBlueprintLibrary::GetTrueComponentType(
                propertyIt.Value);
        int64 componentCount;

        ECesiumMetadataPackedGpuType gpuType =
            ECesiumMetadataPackedGpuType::None;
        if (type == ECesiumMetadataTrueType::Array) {
          gpuType = CesiumMetadataTrueTypeToDefaultPackedGpuType(componentType);
          componentCount =
              UCesiumMetadataPropertyBlueprintLibrary::GetComponentCount(
                  propertyIt.Value);
        } else {
          gpuType = CesiumMetadataTrueTypeToDefaultPackedGpuType(type);
          componentCount = 1;
        }

        if (gpuType == ECesiumMetadataPackedGpuType::None) {
          continue;
        }

        FPropertyDescription& property =
            pFeatureTable->Properties.Emplace_GetRef();
        property.Name = propertyIt.Key;

        switch (componentCount) {
        case 2:
          property.Type = ECesiumPropertyType::Vec2;
          break;
        case 3:
          property.Type = ECesiumPropertyType::Vec3;
          break;
        case 4:
          property.Type = ECesiumPropertyType::Vec4;
          break;
        default:
          property.Type = ECesiumPropertyType::Scalar;
        };

        if (gpuType == ECesiumMetadataPackedGpuType::Uint8) {
          property.ComponentType = ECesiumPropertyComponentType::Uint8;
        } else /*if (gpuType == float)*/ {
          property.ComponentType = ECesiumPropertyComponentType::Float;
        }

        property.Normalized =
            UCesiumMetadataPropertyBlueprintLibrary::IsNormalized(
                propertyIt.Value);
      }
    }
  }
}
