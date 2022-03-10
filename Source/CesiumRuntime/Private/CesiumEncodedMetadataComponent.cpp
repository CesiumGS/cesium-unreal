
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
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionTextureProperty.h"
#include "Materials/MaterialFunctionMaterialLayer.h"
#include "UObject/Package.h"

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
template <typename ObjClass>
static FORCEINLINE ObjClass* LoadObjFromPath(const FName& Path) {
  if (Path == NAME_None)
    return nullptr;

  return Cast<ObjClass>(
      StaticLoadObject(ObjClass::StaticClass(), nullptr, *Path.ToString()));
}

static FORCEINLINE UMaterialFunction* LoadMaterialFunction(const FName& Path) {
  if (Path == NAME_None)
    return nullptr;

  return LoadObjFromPath<UMaterialFunction>(Path);
}

// TODO: position nodes on graph sensibly
// TODO: consider multiple attributes pointing to same table
void UCesiumEncodedMetadataComponent::GenerateMaterial() {
  FString MaterialBaseName = "ML_Material";
  FString PackageName = "/Game/";
  PackageName += MaterialBaseName;
  UPackage* Package = CreatePackage(NULL, *PackageName);

  // Create an unreal material asset
  UMaterialFunctionMaterialLayerFactory* MaterialFactory =
      NewObject<UMaterialFunctionMaterialLayerFactory>();
  UMaterialFunctionMaterialLayer* UnrealMaterial =
      (UMaterialFunctionMaterialLayer*)MaterialFactory->FactoryCreateNew(
          UMaterialFunctionMaterialLayer::StaticClass(),
          Package,
          *MaterialBaseName,
          RF_Standalone | RF_Public | RF_Transactional,
          NULL,
          GWarn);
  FAssetRegistryModule::AssetCreated(UnrealMaterial);
  Package->FullyLoad();
  Package->SetDirtyFlag(true);

  const int32 IncrX = 400;
  const int32 IncrY = 200;
  int32 NodeX = 0;
  int32 NodeY = 0;
  int32 MaxX = 0;
  int32 MaxY = 0;

  UMaterialFunction* SelectTexCoordsFunction = LoadMaterialFunction(
      "/CesiumForUnreal/Materials/MaterialFunctions/CesiumSelectTexCoords.CesiumSelectTexCoords");
  if (!SelectTexCoordsFunction) {
    // err out
  }

  for (const FFeatureTableDescription& featureTable : this->FeatureTables) {
    if (featureTable.AccessType == ECesiumFeatureTableAccessType::Unknown ||
        featureTable.AccessType == ECesiumFeatureTableAccessType::Mixed) {
      continue;
    }

    int32 SectionLeft = NodeX;
    int32 SectionTop = NodeY;

    UMaterialExpressionCustom* FeatureTableLookup =
        NewObject<UMaterialExpressionCustom>(UnrealMaterial);
    FeatureTableLookup->Inputs.Reserve(featureTable.Properties.Num() + 2);
    FeatureTableLookup->Outputs.Reset(featureTable.Properties.Num() + 1);
    FeatureTableLookup->Outputs.Add(FExpressionOutput(TEXT("return")));
    FeatureTableLookup->bShowOutputNameOnPin = true;
    FeatureTableLookup->Description =
        "Resolve properties from " + featureTable.Name;
    UnrealMaterial->FunctionExpressions.Add(FeatureTableLookup);

    if (featureTable.AccessType == ECesiumFeatureTableAccessType::Texture) {
      UMaterialExpressionTextureObjectParameter* FeatureIdTexture =
          NewObject<UMaterialExpressionTextureObjectParameter>(UnrealMaterial);
      FeatureIdTexture->ParameterName =
          FName("FIT_" + featureTable.Name + "_TX");
      FeatureIdTexture->MaterialExpressionEditorX = NodeX;
      FeatureIdTexture->MaterialExpressionEditorY = NodeY;
      UnrealMaterial->FunctionExpressions.Add(FeatureIdTexture);

      FCustomInput& FeatureIdTextureInput = FeatureTableLookup->Inputs[0];
      FeatureIdTextureInput.InputName = "FeatureIdTexture";
      FeatureIdTextureInput.Input.Expression = FeatureIdTexture;

      NodeY += IncrY;

      UMaterialExpressionScalarParameter* TexCoordsIndex =
          NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
      TexCoordsIndex->ParameterName = FName("FIT_" + featureTable.Name + "_UV");
      TexCoordsIndex->DefaultValue = 0.0f;
      TexCoordsIndex->MaterialExpressionEditorX = NodeX;
      TexCoordsIndex->MaterialExpressionEditorY = NodeY;
      UnrealMaterial->FunctionExpressions.Add(TexCoordsIndex);

      NodeX += IncrX;

      UMaterialExpressionMaterialFunctionCall* SelectTexCoords =
          NewObject<UMaterialExpressionMaterialFunctionCall>(UnrealMaterial);
      SelectTexCoords->MaterialFunction = SelectTexCoordsFunction;
      SelectTexCoords->MaterialExpressionEditorX = NodeX;
      SelectTexCoords->MaterialExpressionEditorY = NodeY;

      // TODO: need output?
      TArray<FFunctionExpressionOutput> _;
      SelectTexCoordsFunction->GetInputsAndOutputs(
          SelectTexCoords->FunctionInputs,
          _);
      SelectTexCoords->FunctionInputs[0].Input.Expression = TexCoordsIndex;
      UnrealMaterial->FunctionExpressions.Add(SelectTexCoords);

      FCustomInput& TexCoordsInput =
          FeatureTableLookup->Inputs.Emplace_GetRef();
      TexCoordsInput.InputName = "TexCoords";
      TexCoordsInput.Input.Expression = SelectTexCoords;

      // TODO: use channel mask, instead of hardcoding r channel
      // cannot determine channel name in editor, use channel mask + sample to
      // derive id.
      FeatureTableLookup->Code =
          "uint propertyIndex = asuint(FeatureIdTexture.Sample(FeatureIdTextureSampler, TexCoords).r);\n";
    } else {
      // Create material for vertex attributes

      UMaterialExpressionScalarParameter* AttributeIndex =
          NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
      AttributeIndex->ParameterName = FName("FA_" + featureTable.Name);
      AttributeIndex->DefaultValue = 0.0f;
      AttributeIndex->MaterialExpressionEditorX = NodeX;
      AttributeIndex->MaterialExpressionEditorY = NodeY;
      UnrealMaterial->FunctionExpressions.Add(AttributeIndex);

      NodeX += IncrX;

      UMaterialExpressionMaterialFunctionCall* SelectTexCoords =
          NewObject<UMaterialExpressionMaterialFunctionCall>(UnrealMaterial);
      SelectTexCoords->MaterialFunction = SelectTexCoordsFunction;
      SelectTexCoords->MaterialExpressionEditorX = NodeX;
      SelectTexCoords->MaterialExpressionEditorY = NodeY;

      // TODO: need output?
      TArray<FFunctionExpressionOutput> _;
      SelectTexCoordsFunction->GetInputsAndOutputs(
          SelectTexCoords->FunctionInputs,
          _);
      SelectTexCoords->FunctionInputs[0].Input.Expression = AttributeIndex;
      UnrealMaterial->FunctionExpressions.Add(SelectTexCoords);

      FCustomInput& TexCoordsInput = FeatureTableLookup->Inputs[0];
      TexCoordsInput.InputName = "EncodedPropertyIndex";
      TexCoordsInput.Input.Expression = SelectTexCoords;

      FeatureTableLookup->Code =
          "uint propertyIndex = asuint(EncodedPropertyIndex.r);\n";
    }

    // TODO: vertex attributes

    NodeX = SectionLeft;
    NodeY += IncrY;

    FeatureTableLookup->AdditionalOutputs.Reserve(
        featureTable.Properties.Num());
    for (const FPropertyDescription& property : featureTable.Properties) {
      UMaterialExpressionTextureObjectParameter* PropertyArray =
          NewObject<UMaterialExpressionTextureObjectParameter>(UnrealMaterial);
      PropertyArray->ParameterName =
          FName("FTB_" + featureTable.Name + "_" + property.Name);
      PropertyArray->MaterialExpressionEditorX = NodeX;
      PropertyArray->MaterialExpressionEditorY = NodeY;
      UnrealMaterial->FunctionExpressions.Add(PropertyArray);

      FCustomInput& PropertyInput = FeatureTableLookup->Inputs.Emplace_GetRef();
      PropertyInput.InputName = FName(property.Name + "_array");
      PropertyInput.Input.Expression = PropertyArray;

      FCustomOutput& PropertyOutput =
          FeatureTableLookup->AdditionalOutputs.Emplace_GetRef();
      PropertyOutput.OutputName = FName(property.Name);
      FeatureTableLookup->Outputs.Add(
          FExpressionOutput(PropertyOutput.OutputName));

      FString swizzle = "";
      switch (property.Type) {
      case ECesiumPropertyType::Vec2:
        PropertyOutput.OutputType = ECustomMaterialOutputType::CMOT_Float2;
        swizzle = "rg";
        break;
      case ECesiumPropertyType::Vec3:
        PropertyOutput.OutputType = ECustomMaterialOutputType::CMOT_Float3;
        swizzle = "rgb";
        break;
      case ECesiumPropertyType::Vec4:
        PropertyOutput.OutputType = ECustomMaterialOutputType::CMOT_Float4;
        swizzle = "rgba";
        break;
      // case ECesiumPropertyType::Scalar:
      default:
        PropertyOutput.OutputType = ECustomMaterialOutputType::CMOT_Float1;
        swizzle = "r";
      };

      FeatureTableLookup->Code +=
          property.Name + " = asfloat(" + property.Name +
          "_array.Load(int3(propertyIndex, 0, 0))." + swizzle + ");\n";

      NodeY += IncrY;
    }

    int32 SectionBottom = NodeY;

    FeatureTableLookup->OutputType = ECustomMaterialOutputType::CMOT_Float1;

    FeatureTableLookup->Code += "float propertyIndexF = propertyIndex;\n";
    FeatureTableLookup->Code += "return propertyIndexF;";

    // TODO: why doesn't this link?
    // FeatureTableLookup->RebuildOutputs();

    NodeX += 2 * IncrX;
    NodeY = SectionTop;

    FeatureTableLookup->MaterialExpressionEditorX = NodeX;
    FeatureTableLookup->MaterialExpressionEditorY = NodeY;

    NodeX = SectionLeft;
    NodeY = SectionBottom;
  }

  NodeX = 0;
  NodeY = -IncrY;

  UMaterialExpressionFunctionInput* InputMaterial =
      NewObject<UMaterialExpressionFunctionInput>(UnrealMaterial);

  InputMaterial->InputType =
      EFunctionInputType::FunctionInput_MaterialAttributes;
  InputMaterial->bUsePreviewValueAsDefault = true;
  InputMaterial->MaterialExpressionEditorX = NodeX;
  InputMaterial->MaterialExpressionEditorY = NodeY;
  UnrealMaterial->FunctionExpressions.Add(InputMaterial);

  NodeX += 3 * IncrX;

  UMaterialExpressionFunctionOutput* OutputMaterial =
      NewObject<UMaterialExpressionFunctionOutput>(UnrealMaterial);

  OutputMaterial->MaterialExpressionEditorX = NodeX;
  OutputMaterial->MaterialExpressionEditorY = NodeY;
  OutputMaterial->A = FMaterialAttributesInput();
  OutputMaterial->A.Expression = InputMaterial;
  UnrealMaterial->FunctionExpressions.Add(OutputMaterial);

  // let the material update itself if necessary
  UnrealMaterial->PreEditChange(NULL);
  UnrealMaterial->PostEditChange();

  // make sure that any static meshes, etc using this material will stop using
  // the FMaterialResource of the original material, and will use the new
  // FMaterialResource created when we make a new UMaterial in place
  FGlobalComponentReregisterContext RecreateComponents;
}
#endif
