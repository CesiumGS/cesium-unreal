
#include "CesiumEncodedMetadataComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Cesium3DTileset.h"
#include "CesiumFeatureTextureProperty.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumMetadataConversions.h"
#include "CesiumMetadataModel.h"

#if WITH_EDITOR
#include "ComponentReregisterContext.h"
#include "ContentBrowserModule.h"
#include "Factories/MaterialFunctionMaterialLayerFactory.h"
#include "IContentBrowserSingleton.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSetMaterialAttributes.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionTextureProperty.h"
#include "Materials/MaterialExpressionVertexInterpolator.h"
#include "Materials/MaterialFunctionMaterialLayer.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"
#endif

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
      FFeatureTextureDescription* pFeatureTexture = nullptr;
      for (FFeatureTextureDescription& existingFeatureTexture :
           this->FeatureTextures) {
        if (existingFeatureTexture.Name == featureTextureIt.Key) {
          pFeatureTexture = &existingFeatureTexture;
          break;
        }
      }

      if (!pFeatureTexture) {
        pFeatureTexture = &this->FeatureTextures.Emplace_GetRef();
        pFeatureTexture->Name = featureTextureIt.Key;
      }

      const TArray<FString>& propertyNames =
          UCesiumFeatureTextureBlueprintLibrary::GetPropertyKeys(
              featureTextureIt.Value);

      for (const FString& propertyName : propertyNames) {
        bool propertyExists = false;
        for (const FFeatureTexturePropertyDescription& existingProperty :
             pFeatureTexture->Properties) {
          if (existingProperty.Name == propertyName) {
            propertyExists = true;
            break;
          }
        }

        if (!propertyExists) {
          FCesiumFeatureTextureProperty property =
              UCesiumFeatureTextureBlueprintLibrary::FindProperty(
                  featureTextureIt.Value,
                  propertyName);
          FFeatureTexturePropertyDescription& propertyDescription =
              pFeatureTexture->Properties.Emplace_GetRef();
          propertyDescription.Name = propertyName;
          propertyDescription.Normalized =
              UCesiumFeatureTexturePropertyBlueprintLibrary::IsNormalized(
                  property);

          switch (
              UCesiumFeatureTexturePropertyBlueprintLibrary::GetComponentCount(
                  property)) {
          case 2:
            propertyDescription.Type = ECesiumPropertyType::Vec2;
            break;
          case 3:
            propertyDescription.Type = ECesiumPropertyType::Vec3;
            break;
          case 4:
            propertyDescription.Type = ECesiumPropertyType::Vec4;
            break;
          // case 1:
          default:
            propertyDescription.Type = ECesiumPropertyType::Scalar;
          }

          propertyDescription.Swizzle =
              UCesiumFeatureTexturePropertyBlueprintLibrary::GetSwizzle(
                  property);
        }
      }
    }
  }

  for (const UActorComponent* pComponent : pOwner->GetComponents()) {
    const UCesiumGltfPrimitiveComponent* pGltfPrimitive =
        Cast<UCesiumGltfPrimitiveComponent>(pComponent);
    if (!pGltfPrimitive) {
      continue;
    }

    const FCesiumMetadataPrimitive& primitive = pGltfPrimitive->Metadata;
    const TArray<FCesiumFeatureIdAttribute>& attributes =
        UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureIdAttributes(
            primitive);
    const TArray<FCesiumFeatureIdTexture>& textures =
        UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureIdTextures(
            primitive);

    for (const FCesiumFeatureIdAttribute& attribute : attributes) {
      const FString& featureTableName =
          UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureTableName(
              attribute);
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

    for (const FCesiumFeatureIdTexture& texture : textures) {
      const FString& featureTableName =
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureTableName(texture);
      for (FFeatureTableDescription& featureTable : this->FeatureTables) {
        if (featureTableName == featureTable.Name) {
          if (featureTable.AccessType ==
              ECesiumFeatureTableAccessType::Unknown) {
            featureTable.AccessType = ECesiumFeatureTableAccessType::Texture;
            switch (texture.getFeatureIdTextureView().getChannel()) {
            case 1:
              featureTable.Channel = "g";
              break;
            case 2:
              featureTable.Channel = "b";
              break;
            case 3:
              featureTable.Channel = "a";
              break;
            // case 0:
            default:
              featureTable.Channel = "r";
            }
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

// Not exhaustive in fixing unsafe names. Add more functionality here as needed
// when in-compatible metadata names arise as recurring problems.
static FString createHlslSafeName(const FString& rawName) {
  FString safeName = rawName;
  safeName.ReplaceCharInline(':', '_', ESearchCase::Type::IgnoreCase);
  return safeName;
}

void UCesiumEncodedMetadataComponent::GenerateMaterial() {
  ACesium3DTileset* pTileset = Cast<ACesium3DTileset>(this->GetOwner());

  if (!pTileset) {
    return;
  }

  FString MaterialBaseName = pTileset->GetFName().ToString() + "_Metadata_ML";
  FString MaterialName = MaterialBaseName;
  FString PackageBaseName = "/Game/";
  FString PackageName = PackageBaseName + MaterialName;

  int MaterialNameIndex = 0;
  while (FPackageName::DoesPackageExist(PackageName)) {
    if (++MaterialNameIndex == 10) {
      return;
    }

    MaterialName = MaterialBaseName + FString::FromInt(MaterialNameIndex);
    PackageName = PackageBaseName + MaterialName;
  }

  UMaterialFunction* SelectTexCoordsFunction = LoadMaterialFunction(
      "/CesiumForUnreal/Materials/MaterialFunctions/CesiumSelectTexCoords.CesiumSelectTexCoords");
  if (!SelectTexCoordsFunction) {
    return;
  }

  UPackage* Package = CreatePackage(*PackageName);

  // Create an unreal material asset
  UMaterialFunctionMaterialLayerFactory* MaterialFactory =
      NewObject<UMaterialFunctionMaterialLayerFactory>();
  UMaterialFunctionMaterialLayer* UnrealMaterial =
      (UMaterialFunctionMaterialLayer*)MaterialFactory->FactoryCreateNew(
          UMaterialFunctionMaterialLayer::StaticClass(),
          Package,
          *MaterialName,
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

      NodeX += IncrX;

      // TODO: Should the channel mask be determined dynamically instead of at
      // editor-time like it is now?
      FeatureTableLookup->Code =
          "uint propertyIndex = asuint(FeatureIdTexture.Sample(FeatureIdTextureSampler, TexCoords)." +
          featureTable.Channel + ");\n";

      FeatureTableLookup->MaterialExpressionEditorX = NodeX;
      FeatureTableLookup->MaterialExpressionEditorY = NodeY;
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

      TArray<FFunctionExpressionOutput> _;
      SelectTexCoordsFunction->GetInputsAndOutputs(
          SelectTexCoords->FunctionInputs,
          _);
      SelectTexCoords->FunctionInputs[0].Input.Expression = AttributeIndex;
      UnrealMaterial->FunctionExpressions.Add(SelectTexCoords);

      FCustomInput& TexCoordsInput = FeatureTableLookup->Inputs[0];
      TexCoordsInput.InputName = "PropertyIndexUV";
      TexCoordsInput.Input.Expression = SelectTexCoords;

      NodeX += IncrX;

      FeatureTableLookup->Code = "uint propertyIndex = PropertyIndexUV.r;\n";

      FeatureTableLookup->MaterialExpressionEditorX = NodeX;
      FeatureTableLookup->MaterialExpressionEditorY = NodeY;

      NodeX += IncrX;

      UMaterialExpressionVertexInterpolator* Interpolator =
          NewObject<UMaterialExpressionVertexInterpolator>(UnrealMaterial);
      Interpolator->MaterialExpressionEditorX = NodeX;
      Interpolator->MaterialExpressionEditorY = NodeY;
      UnrealMaterial->FunctionExpressions.Add(Interpolator);
    }

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

      FString propertyName = createHlslSafeName(property.Name);

      FCustomInput& PropertyInput = FeatureTableLookup->Inputs.Emplace_GetRef();
      FString propertyArrayName = property.Name + "_array";
      PropertyInput.InputName = FName(propertyArrayName);
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

      FString componentTypeInterpretation =
          property.ComponentType == ECesiumPropertyComponentType::Float
              ? "asfloat"
              : "asuint";

      FeatureTableLookup->Code +=
          propertyName + " = " + componentTypeInterpretation + "(" +
          propertyArrayName + ".Load(int3(propertyIndex, 0, 0))." + swizzle +
          ");\n";

      NodeY += IncrY;
    }

    FeatureTableLookup->OutputType = ECustomMaterialOutputType::CMOT_Float1;

    FeatureTableLookup->Code += "float propertyIndexF = propertyIndex;\n";
    FeatureTableLookup->Code += "return propertyIndexF;";

    NodeX = SectionLeft;
  }

  for (const FFeatureTextureDescription& featureTexture :
       this->FeatureTextures) {
    int32 SectionLeft = NodeX;
    int32 SectionTop = NodeY;

    UMaterialExpressionCustom* FeatureTextureLookup =
        NewObject<UMaterialExpressionCustom>(UnrealMaterial);
    FeatureTextureLookup->Inputs.Reset(2 * featureTexture.Properties.Num());
    FeatureTextureLookup->Outputs.Reset(featureTexture.Properties.Num() + 1);
    FeatureTextureLookup->Outputs.Add(FExpressionOutput(TEXT("return")));
    FeatureTextureLookup->bShowOutputNameOnPin = true;
    FeatureTextureLookup->Code = "";
    FeatureTextureLookup->Description =
        "Resolve properties from " + featureTexture.Name;
    FeatureTextureLookup->MaterialExpressionEditorX = NodeX + 2 * IncrX;
    FeatureTextureLookup->MaterialExpressionEditorY = NodeY;
    UnrealMaterial->FunctionExpressions.Add(FeatureTextureLookup);

    for (const FFeatureTexturePropertyDescription& property :
         featureTexture.Properties) {
      UMaterialExpressionTextureObjectParameter* PropertyTexture =
          NewObject<UMaterialExpressionTextureObjectParameter>(UnrealMaterial);
      PropertyTexture->ParameterName =
          FName("FTX_" + featureTexture.Name + "_" + property.Name + "_TX");
      PropertyTexture->MaterialExpressionEditorX = NodeX;
      PropertyTexture->MaterialExpressionEditorY = NodeY;
      UnrealMaterial->FunctionExpressions.Add(PropertyTexture);

      FString propertyName = createHlslSafeName(property.Name);

      FCustomInput& PropertyTextureInput =
          FeatureTextureLookup->Inputs.Emplace_GetRef();
      FString propertyTextureName = propertyName + "_TX";
      PropertyTextureInput.InputName = FName(propertyTextureName);
      PropertyTextureInput.Input.Expression = PropertyTexture;

      NodeY += IncrY;

      UMaterialExpressionScalarParameter* TexCoordsIndex =
          NewObject<UMaterialExpressionScalarParameter>(UnrealMaterial);
      TexCoordsIndex->ParameterName =
          FName("FTX_" + featureTexture.Name + "_" + property.Name + "_UV");
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

      TArray<FFunctionExpressionOutput> _;
      SelectTexCoordsFunction->GetInputsAndOutputs(
          SelectTexCoords->FunctionInputs,
          _);
      SelectTexCoords->FunctionInputs[0].Input.Expression = TexCoordsIndex;
      UnrealMaterial->FunctionExpressions.Add(SelectTexCoords);

      FCustomInput& TexCoordsInput =
          FeatureTextureLookup->Inputs.Emplace_GetRef();
      FString propertyUvName = propertyName + "_UV";
      TexCoordsInput.InputName = FName(propertyUvName);
      TexCoordsInput.Input.Expression = SelectTexCoords;

      FCustomOutput& PropertyOutput =
          FeatureTextureLookup->AdditionalOutputs.Emplace_GetRef();
      PropertyOutput.OutputName = FName(propertyName);
      FeatureTextureLookup->Outputs.Add(
          FExpressionOutput(PropertyOutput.OutputName));

      // Either the property is normalized or it is coerced into float. Either
      // way, the outputs will be float type.
      switch (property.Type) {
      case ECesiumPropertyType::Vec2:
        PropertyOutput.OutputType = ECustomMaterialOutputType::CMOT_Float2;
        break;
      case ECesiumPropertyType::Vec3:
        PropertyOutput.OutputType = ECustomMaterialOutputType::CMOT_Float3;
        break;
      case ECesiumPropertyType::Vec4:
        PropertyOutput.OutputType = ECustomMaterialOutputType::CMOT_Float4;
        break;
      // case ECesiumPropertyType::Scalar:
      default:
        PropertyOutput.OutputType = ECustomMaterialOutputType::CMOT_Float1;
      };

      // TODO: should dynamic channel offsets be used instead of swizzle string
      // determined at editor time? E.g. can swizzles be different for the same
      // property texture on different tiles?
      FeatureTextureLookup->Code +=
          propertyName + " = " +
          (property.Normalized ? "asfloat(" : "asuint(") + propertyTextureName +
          ".Sample(" + propertyTextureName + "Sampler, " + propertyUvName +
          ")." + property.Swizzle + ");\n";

      NodeY += IncrY;
    }

    FeatureTextureLookup->OutputType = ECustomMaterialOutputType::CMOT_Float1;
    FeatureTextureLookup->Code += "return 0.0f;";

    NodeX = SectionLeft;
  }

  NodeY = -IncrY;

  UMaterialExpressionFunctionInput* InputMaterial =
      NewObject<UMaterialExpressionFunctionInput>(UnrealMaterial);
  InputMaterial->InputType =
      EFunctionInputType::FunctionInput_MaterialAttributes;
  InputMaterial->bUsePreviewValueAsDefault = true;
  InputMaterial->MaterialExpressionEditorX = NodeX;
  InputMaterial->MaterialExpressionEditorY = NodeY;
  UnrealMaterial->FunctionExpressions.Add(InputMaterial);

  NodeX += IncrX;

  UMaterialExpressionSetMaterialAttributes* SetMaterialAttributes =
      NewObject<UMaterialExpressionSetMaterialAttributes>(UnrealMaterial);
  SetMaterialAttributes->Inputs[0].Expression = InputMaterial;
  SetMaterialAttributes->MaterialExpressionEditorX = NodeX;
  SetMaterialAttributes->MaterialExpressionEditorY = NodeY;
  UnrealMaterial->FunctionExpressions.Add(SetMaterialAttributes);

  NodeX += IncrX;

  UMaterialExpressionFunctionOutput* OutputMaterial =
      NewObject<UMaterialExpressionFunctionOutput>(UnrealMaterial);
  OutputMaterial->MaterialExpressionEditorX = NodeX;
  OutputMaterial->MaterialExpressionEditorY = NodeY;
  OutputMaterial->A = FMaterialAttributesInput();
  OutputMaterial->A.Expression = SetMaterialAttributes;
  UnrealMaterial->FunctionExpressions.Add(OutputMaterial);

  // let the material update itself if necessary
  UnrealMaterial->PreEditChange(NULL);
  UnrealMaterial->PostEditChange();

  // make sure that any static meshes, etc using this material will stop using
  // the FMaterialResource of the original material, and will use the new
  // FMaterialResource created when we make a new UMaterial in place
  FGlobalComponentReregisterContext RecreateComponents;

  TArray<UObject*> AssetsToHighlight;
  AssetsToHighlight.Add(UnrealMaterial);

  FContentBrowserModule& ContentBrowserModule =
      FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(
          "ContentBrowser");
  ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToHighlight);
}
#endif
