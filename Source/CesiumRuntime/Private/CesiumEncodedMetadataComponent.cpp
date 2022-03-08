
#include "CesiumEncodedMetadataComponent.h"
#include "Cesium3DTileset.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumMetadataConversions.h"
#include "CesiumMetadataModel.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "ComponentReregisterContext.h"
#include "Factories/MaterialFactoryNew.h"
#include "Factories/MaterialFunctionMaterialLayerFactory.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureProperty.h"
#include "Materials/MaterialExpressionTextureSampleParameter.h"
#include "Materials/MaterialFunctionMaterialLayer.h"
#include "UObject/Package.h"

#include "Materials/MaterialExpressionTextureCoordinate.h"

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

    for (const auto& featureTextureIt : featureTextures) {
      bool found = false;
      for (const FFeatureTextureDescription& existingFeatureTexture :
           this->FeatureTextures) {
        if (existingFeatureTexture.Name == featureTextureIt.Key) {
          found = true;
          break;
        }
      }

      if (found) {
        continue;
      }

      // TODO: add more checks than just name for feature textures?
      this->FeatureTextures.Emplace(
          FFeatureTextureDescription{featureTextureIt.Key});
    }
  }

  for (const UActorComponent* pComponent : pOwner->GetComponents()) {
    const UCesiumGltfPrimitiveComponent* pGltfPrimitive =
        Cast<UCesiumGltfPrimitiveComponent>(pComponent);
    if (!pGltfPrimitive) {
      continue;
    }

    const FCesiumMetadataPrimitive& primitive = pGltfPrimitive->Metadata;
    const TArray<FCesiumVertexMetadata>& attributes =
        UCesiumMetadataPrimitiveBlueprintLibrary::GetVertexFeatures(primitive);
    const TArray<FCesiumFeatureIDTexture>& textures =
        UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureIDTextures(
            primitive);

    for (const FCesiumVertexMetadata& attribute : attributes) {
      const FString& featureTableName =
          UCesiumVertexMetadataBlueprintLibrary::GetFeatureTableName(attribute);
      for (FFeatureTableDescription& featureTable : this->FeatureTables) {
        if (featureTableName == featureTable.Name) {
          if (featureTable.AccessType ==
              ECesiumFeatureTableAccessType::Unknown) {
            featureTable.AccessType = ECesiumFeatureTableAccessType::Attribute;
          }

          break;
        }
      }
    }

    for (const FCesiumFeatureIDTexture& texture : textures) {
      const FString& featureTableName =
          UCesiumFeatureIDTextureBlueprintLibrary::GetFeatureTableName(texture);
      for (FFeatureTableDescription& featureTable : this->FeatureTables) {
        if (featureTableName == featureTable.Name) {
          if (featureTable.AccessType ==
              ECesiumFeatureTableAccessType::Unknown) {
            featureTable.AccessType = ECesiumFeatureTableAccessType::Texture;
          } else if (
              featureTable.AccessType ==
              ECesiumFeatureTableAccessType::Attribute) {
            featureTable.AccessType = ECesiumFeatureTableAccessType::Mixed;
          }

          break;
        }
      }
    }
  }
}

#if WITH_EDITOR
static UMaterialExpressionCustom* CreateSelectTexCoords(
    UMaterialFunctionMaterialLayer* UnrealMaterial,
    UMaterialExpressionScalarParameter* TexCoordIndex) {
  // TODO: this is a temporary substitute
  // for figuring out how to load the existing Cesium material function

  TArray<UMaterialExpressionTextureCoordinate*> TexCoords;
  TexCoords.SetNum(8);
  for (int32 i = 0; i < 8; ++i) {
    UMaterialExpressionTextureCoordinate* TexCoord =
        NewObject<UMaterialExpressionTextureCoordinate>(UnrealMaterial);
    TexCoord->CoordinateIndex = i;
    UnrealMaterial->FunctionExpressions.Add(TexCoord);
    TexCoords[i] = TexCoord;
  }

  UMaterialExpressionCustom* Select =
      NewObject<UMaterialExpressionCustom>(UnrealMaterial);
  Select->OutputType = ECustomMaterialOutputType::CMOT_Float2;
  Select->Inputs.SetNum(9);

  Select->Inputs[0].InputName = "TexCoordIndex";
  Select->Inputs[0].Input.Expression = TexCoordIndex;

  for (int32 i = 0; i < 8; ++i) {
    Select->Inputs[i + 1].InputName = FName("TexCoord" + FString::FromInt(i));
    Select->Inputs[i + 1].Input.Expression = TexCoords[i];
  }

  Select->Code =
      "float2 texCoords[8] = {TexCoord0, TexCoord1, TexCoord2, TexCoord3, TexCoord4, TexCoord5, TexCoord6, TexCoord7}; int index = TexCoordIndex; return texCoords[index];";

  UnrealMaterial->FunctionExpressions.Add(Select);
  return Select;
}

template <typename ObjClass>
static FORCEINLINE ObjClass* LoadObjFromPath(const FName& Path) {
  if (Path == NAME_None)
    return nullptr;

  return Cast<ObjClass>(
      StaticLoadObject(ObjClass::StaticClass(), nullptr, *Path.ToString()));
}

static FORCEINLINE UMaterial* LoadMaterialFromPath(const FName& Path) {
  if (Path == NAME_None)
    return nullptr;

  return LoadObjFromPath<UMaterial>(Path);
}

void UCesiumEncodedMetadataComponent::GenerateMaterial() {
  FString MaterialBaseName = "ML_Material";
  FString PackageName = "/Game/";
  PackageName += MaterialBaseName;
  UPackage* Package = CreatePackage(NULL, *PackageName);

  // Create an unreal material asset
  auto MaterialFactory = NewObject<UMaterialFunctionMaterialLayerFactory>();
  UMaterialFunctionMaterialLayer* UnrealMaterial =
      (UMaterialFunctionMaterialLayer*)MaterialFactory->FactoryCreateNew(
          UMaterialFunctionMaterialLayer::StaticClass(),
          Package,
          *MaterialBaseName,
          RF_Standalone | RF_Public,
          NULL,
          GWarn);
  FAssetRegistryModule::AssetCreated(UnrealMaterial);
  Package->FullyLoad();
  Package->SetDirtyFlag(true);

  UMaterialExpressionScalarParameter* TexCoordsIndex =
      NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
  TexCoordsIndex->ParameterName = "FeatureIdTexture:0_TextureCoordinateIndex";
  TexCoordsIndex->DefaultValue = 0.0f;
  UnrealMaterial->FunctionExpressions.Add(TexCoordsIndex);

  UMaterialExpressionCustom* SelectTexCoords =
      CreateSelectTexCoords(UnrealMaterial, TexCoordsIndex);

  // UMaterial *

  // let the material update itself if necessary
  UnrealMaterial->PreEditChange(NULL);
  UnrealMaterial->PostEditChange();

  // make sure that any static meshes, etc using this material will stop using
  // the FMaterialResource of the original material, and will use the new
  // FMaterialResource created when we make a new UMaterial in place
  FGlobalComponentReregisterContext RecreateComponents;

  /*
    UMaterialExpressionTextureSampleParameter* FeatureIdTexture =
        NewObject<MaterialExpressionTextureSampleParameter>(UnrealMaterial);
    FeatureIdTexture->ParameterName = "FeatureIdTexture:0_Texture";
    UnrealMaterial->Expressions.Add(FeatureIdTexture);

    UMaterialExpressionTextureSampleParameter* FeatureTable =
        NewObject<MaterialExpressionTextureSampleParameter>(UnrealMaterial);
    FeatureTable->ParameterName = "FeatureTable:labels_attribute";
    UnrealMaterial->Expressions.Add(FeatureTable);

    UMaterialExpressionCustom* PropertyArrayLookup =
        NewObject<UMaterialExpressionCustom>(UnrealMaterial);
    FeatureTableLookup->OutputType = ECustomMaterialOutputType::CMOT_Float1;*/
}
#endif
