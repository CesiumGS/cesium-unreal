
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
#include "Containers/Map.h"
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
    const TMap<FString, FCesiumFeatureTable>& featureTables =
        UCesiumMetadataModelBlueprintLibrary::GetFeatureTables(model);
    const TMap<FString, FCesiumFeatureTexture>& featureTextures =
        UCesiumMetadataModelBlueprintLibrary::GetFeatureTextures(model);

    for (const auto& featureTableIt : featureTables) {
      const TMap<FString, FCesiumMetadataProperty>& properties =
          UCesiumFeatureTableBlueprintLibrary::GetProperties(
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

static FORCEINLINE UMaterialFunctionMaterialLayer* LoadMaterialLayer(const FName& Path) {
  if (Path == NAME_None)
    return nullptr;

  return LoadObjFromPath<UMaterialFunctionMaterialLayer>(Path);
}

// Not exhaustive in fixing unsafe names. Add more functionality here as needed
// when in-compatible metadata names arise as recurring problems.
static FString createHlslSafeName(const FString& rawName) {
  FString safeName = rawName;
  safeName.ReplaceCharInline(':', '_', ESearchCase::Type::IgnoreCase);
  return safeName;
}

// Seperate nodes into auto-generated and user-added. Collect the property 
// result nodes.
static void ClassifyNodes(
    UMaterialFunctionMaterialLayer* Layer,
    TArray<UMaterialExpression*>& AutoGeneratedNodes,
    TArray<UMaterialExpression*>& UserAddedNodes,
    TArray<UMaterialExpressionCustom*>& ResultNodes) {

  for (UMaterialExpression* Node : Layer->FunctionExpressions) {
    // Check if this node has the Cesium Auto-Generated prefix (CAG_).
    if (Node->Desc.Mid(0, 4) == "CAG_") {
      AutoGeneratedNodes.Add(Node);

      // The only auto-generated custom nodes are the property result nodes.
      UMaterialExpressionCustom* CustomNode = Cast<UMaterialExpressionCustom>(Node);
      if (CustomNode) {
        ResultNodes.Add(CustomNode);
      }
    } else {
      UserAddedNodes.Add(Node);
    }
  }
}

static void ClearAutoGeneratedNodes(
    UMaterialFunctionMaterialLayer* Layer,
    TMap<FString, TArray<FExpressionInput*>>& ConnectionRemap) {

  TArray<UMaterialExpression*> AutoGeneratedNodes;
  TArray<UMaterialExpression*> UserAddedNodes;
  TArray<UMaterialExpressionCustom*> ResultNodes;
  ClassifyNodes(Layer, AutoGeneratedNodes, UserAddedNodes, ResultNodes);

  // Determine which user-added connections to remap when regenerating the 
  // auto-generated nodes.
  for (const UMaterialExpressionCustom* ResultNode : ResultNodes) {
    int32 OutputIndex = 0;
    for (const FExpressionOutput& PropertyOutput : ResultNode->Outputs) {
      FString Key = ResultNode->Description + PropertyOutput.OutputName.ToString();
      
      // Look for user-made connections to this property.
      TArray<FExpressionInput*> Connections;
      for (UMaterialExpression* UserNode : UserAddedNodes) {
        for (FExpressionInput* Input : UserNode->GetInputs()) {
          if (Input->Expression == ResultNode &&
              Input->OutputIndex == OutputIndex) {
            Connections.Add(Input);
            Input->Expression = nullptr;
          }
        }
      }

      ConnectionRemap.Emplace(MoveTemp(Key), MoveTemp(Connections));
      ++OutputIndex;
    }
  }
  
  // Remove auto-generated nodes.
  for (UMaterialExpression* AutoGeneratedNode: AutoGeneratedNodes) {
    Layer->FunctionExpressions.Remove(AutoGeneratedNode);
  }
}

static void RemapUserConnections(
    UMaterialFunctionMaterialLayer* Layer,
    TMap<FString, TArray<FExpressionInput*>>& ConnectionRemap) {
  
  TArray<UMaterialExpression*> AutoGeneratedNodes;
  TArray<UMaterialExpression*> UserAddedNodes;
  TArray<UMaterialExpressionCustom*> ResultNodes;
  ClassifyNodes(Layer, AutoGeneratedNodes, UserAddedNodes, ResultNodes);

  for (UMaterialExpressionCustom* ResultNode : ResultNodes) {
    int32 OutputIndex = 0;
    for (const FExpressionOutput& PropertyOutput : ResultNode->Outputs) {
      FString Key = ResultNode->Description + PropertyOutput.OutputName.ToString();

      TArray<FExpressionInput*>* pConnections = ConnectionRemap.Find(Key);
      if (pConnections) {
        for (FExpressionInput* pConnection : *pConnections) {
          pConnection->Expression = ResultNode;
          pConnection->OutputIndex = OutputIndex;
        }
      }

      ++OutputIndex;
    }
  }
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

  /*
  int MaterialNameIndex = 0;
  while (FPackageName::DoesPackageExist(PackageName)) {
    if (++MaterialNameIndex == 10) {
      return;
    }

    MaterialName = MaterialBaseName + FString::FromInt(MaterialNameIndex);
    PackageName = PackageBaseName + MaterialName;
  }*/

  bool Overwriting = false;
  UMaterialFunctionMaterialLayer* UnrealMaterial = nullptr;
  if (FPackageName::DoesPackageExist(PackageName)) {
    Overwriting = true;
    UnrealMaterial = LoadMaterialLayer(FName(PackageName));
  }

  UMaterialFunction* SelectTexCoordsFunction = LoadMaterialFunction(
      "/CesiumForUnreal/Materials/MaterialFunctions/CesiumSelectTexCoords.CesiumSelectTexCoords");
  if (!SelectTexCoordsFunction) {
    return;
  }

  if (!UnrealMaterial) {
    UPackage* Package = CreatePackage(*PackageName);

    // Create an unreal material asset
    UMaterialFunctionMaterialLayerFactory* MaterialFactory =
        NewObject<UMaterialFunctionMaterialLayerFactory>();
    UnrealMaterial =
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
  }

  UnrealMaterial->PreEditChange(NULL);

  TMap<FString, TArray<FExpressionInput*>> ConnectionRemap;
  ClearAutoGeneratedNodes(UnrealMaterial, ConnectionRemap);

  TArray<UMaterialExpression*> AutoGeneratedNodes;
  TArray<UMaterialExpression*> OneTimeGeneratedNodes;

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
    AutoGeneratedNodes.Add(FeatureTableLookup);

    if (featureTable.AccessType == ECesiumFeatureTableAccessType::Texture) {
      UMaterialExpressionTextureObjectParameter* FeatureIdTexture =
          NewObject<UMaterialExpressionTextureObjectParameter>(UnrealMaterial);
      FeatureIdTexture->ParameterName =
          FName("FIT_" + featureTable.Name + "_TX");
      FeatureIdTexture->MaterialExpressionEditorX = NodeX;
      FeatureIdTexture->MaterialExpressionEditorY = NodeY;
      AutoGeneratedNodes.Add(FeatureIdTexture);

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
      AutoGeneratedNodes.Add(TexCoordsIndex);

      NodeX += IncrX;

      UMaterialExpressionMaterialFunctionCall* SelectTexCoords =
          NewObject<UMaterialExpressionMaterialFunctionCall>(UnrealMaterial);
      SelectTexCoords->MaterialFunction = SelectTexCoordsFunction;
      SelectTexCoords->MaterialExpressionEditorX = NodeX;
      SelectTexCoords->MaterialExpressionEditorY = NodeY;

      SelectTexCoordsFunction->GetInputsAndOutputs(
          SelectTexCoords->FunctionInputs,
          SelectTexCoords->FunctionOutputs);
      SelectTexCoords->FunctionInputs[0].Input.Expression = TexCoordsIndex;
      AutoGeneratedNodes.Add(SelectTexCoords);

      FCustomInput& TexCoordsInput =
          FeatureTableLookup->Inputs.Emplace_GetRef();
      TexCoordsInput.InputName = FName("TexCoords");
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
      AutoGeneratedNodes.Add(AttributeIndex);

      NodeX += IncrX;

      UMaterialExpressionMaterialFunctionCall* SelectTexCoords =
          NewObject<UMaterialExpressionMaterialFunctionCall>(UnrealMaterial);
      SelectTexCoords->MaterialFunction = SelectTexCoordsFunction;
      SelectTexCoords->MaterialExpressionEditorX = NodeX;
      SelectTexCoords->MaterialExpressionEditorY = NodeY;

      SelectTexCoordsFunction->GetInputsAndOutputs(
          SelectTexCoords->FunctionInputs,
          SelectTexCoords->FunctionOutputs);
      SelectTexCoords->FunctionInputs[0].Input.Expression = AttributeIndex;
      AutoGeneratedNodes.Add(SelectTexCoords);

      FCustomInput& TexCoordsInput = FeatureTableLookup->Inputs[0];
      TexCoordsInput.InputName = FName("PropertyIndexUV");
      TexCoordsInput.Input.Expression = SelectTexCoords;

      NodeX += IncrX;

      FeatureTableLookup->Code = "uint propertyIndex = PropertyIndexUV.r;\n";

      FeatureTableLookup->MaterialExpressionEditorX = NodeX;
      FeatureTableLookup->MaterialExpressionEditorY = NodeY;
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
      AutoGeneratedNodes.Add(PropertyArray);

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
    AutoGeneratedNodes.Add(FeatureTextureLookup);

    for (const FFeatureTexturePropertyDescription& property :
         featureTexture.Properties) {
      UMaterialExpressionTextureObjectParameter* PropertyTexture =
          NewObject<UMaterialExpressionTextureObjectParameter>(UnrealMaterial);
      PropertyTexture->ParameterName =
          FName("FTX_" + featureTexture.Name + "_" + property.Name + "_TX");
      PropertyTexture->MaterialExpressionEditorX = NodeX;
      PropertyTexture->MaterialExpressionEditorY = NodeY;
      AutoGeneratedNodes.Add(PropertyTexture);

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
      AutoGeneratedNodes.Add(TexCoordsIndex);

      NodeX += IncrX;

      UMaterialExpressionMaterialFunctionCall* SelectTexCoords =
          NewObject<UMaterialExpressionMaterialFunctionCall>(UnrealMaterial);
      SelectTexCoords->MaterialFunction = SelectTexCoordsFunction;
      SelectTexCoords->MaterialExpressionEditorX = NodeX;
      SelectTexCoords->MaterialExpressionEditorY = NodeY;

      SelectTexCoordsFunction->GetInputsAndOutputs(
          SelectTexCoords->FunctionInputs,
          SelectTexCoords->FunctionOutputs);
      SelectTexCoords->FunctionInputs[0].Input.Expression = TexCoordsIndex;
      AutoGeneratedNodes.Add(SelectTexCoords);

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

  UMaterialExpressionFunctionInput* InputMaterial = nullptr;
  if (!Overwriting) {
    InputMaterial =
        NewObject<UMaterialExpressionFunctionInput>(UnrealMaterial);
    InputMaterial->InputType =
        EFunctionInputType::FunctionInput_MaterialAttributes;
    InputMaterial->bUsePreviewValueAsDefault = true;
    InputMaterial->MaterialExpressionEditorX = NodeX;
    InputMaterial->MaterialExpressionEditorY = NodeY;
    OneTimeGeneratedNodes.Add(InputMaterial);
  }

  bool VertexInterpolatorExists = false;
  for (UMaterialExpression* ExistingNode :
        UnrealMaterial->FunctionExpressions) {
    if (Cast<UMaterialExpressionVertexInterpolator>(ExistingNode)) {
      VertexInterpolatorExists = true;
      break;
    }
  }

  bool AtleastOneFeatureIdAttribute = false;
  for (const FFeatureTableDescription& featureTable : this->FeatureTables) {
    if (featureTable.AccessType == ECesiumFeatureTableAccessType::Attribute) {
      AtleastOneFeatureIdAttribute = true;
      break;
    }
  }

  NodeX += 2 * IncrX;

  if (!VertexInterpolatorExists && AtleastOneFeatureIdAttribute) {
    UMaterialExpressionVertexInterpolator* Interpolator =
        NewObject<UMaterialExpressionVertexInterpolator>(UnrealMaterial);
    Interpolator->MaterialExpressionEditorX = NodeX;
    Interpolator->MaterialExpressionEditorY = NodeY;
    OneTimeGeneratedNodes.Add(Interpolator);
  }

  NodeX += 2 * IncrX;

  if (!Overwriting) {
    UMaterialExpressionSetMaterialAttributes* SetMaterialAttributes =
        NewObject<UMaterialExpressionSetMaterialAttributes>(UnrealMaterial);
    SetMaterialAttributes->Inputs[0].Expression = InputMaterial;
    SetMaterialAttributes->MaterialExpressionEditorX = NodeX;
    SetMaterialAttributes->MaterialExpressionEditorY = NodeY;
    OneTimeGeneratedNodes.Add(SetMaterialAttributes);

    NodeX += IncrX;

    UMaterialExpressionFunctionOutput* OutputMaterial =
        NewObject<UMaterialExpressionFunctionOutput>(UnrealMaterial);
    OutputMaterial->MaterialExpressionEditorX = NodeX;
    OutputMaterial->MaterialExpressionEditorY = NodeY;
    OutputMaterial->A = FMaterialAttributesInput();
    OutputMaterial->A.Expression = SetMaterialAttributes;
    OneTimeGeneratedNodes.Add(OutputMaterial);
  }

  for (UMaterialExpression* AutoGeneratedNode : AutoGeneratedNodes) {
    UnrealMaterial->FunctionExpressions.Add(AutoGeneratedNode);
    // Mark as auto-generated, prefix the description with CAG_ (for Cesium Auto-Generated).
    // Completely open to suggestions of how else to mark custom information on these assets :)
    AutoGeneratedNode->Desc = "CAG_" + AutoGeneratedNode->Desc; 
  }

  for (UMaterialExpression* OneTimeGeneratedNode : OneTimeGeneratedNodes) {
    UnrealMaterial->FunctionExpressions.Add(OneTimeGeneratedNode);
  }

  RemapUserConnections(UnrealMaterial, ConnectionRemap);

  // let the material update itself if necessary
  //UnrealMaterial->PreEditChange(NULL);
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
