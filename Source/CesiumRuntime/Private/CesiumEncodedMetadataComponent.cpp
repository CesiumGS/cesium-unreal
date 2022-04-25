// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumEncodedMetadataComponent.h"
#include "Cesium3DTileset.h"
#include "CesiumEncodedMetadataUtility.h"
#include "CesiumFeatureTextureProperty.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumMetadataConversions.h"
#include "CesiumMetadataModel.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ComponentReregisterContext.h"
#include "Containers/Map.h"
#include "ContentBrowserModule.h"
#include "Factories/MaterialFunctionMaterialLayerFactory.h"
#include "IContentBrowserSingleton.h"
#include "IMaterialEditor.h"
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
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/Package.h"

extern UNREALED_API class UEditorEngine* GEditor;
#endif

using namespace CesiumEncodedMetadataUtility;

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

      FFeatureTableDescription* pFeatureTable =
          this->FeatureTables.FindByPredicate(
              [&featureTableName = featureTableIt.Key](
                  const FFeatureTableDescription& existingFeatureTable) {
                return existingFeatureTable.Name == featureTableName;
              });

      if (!pFeatureTable) {
        pFeatureTable = &this->FeatureTables.Emplace_GetRef();
        pFeatureTable->Name = featureTableIt.Key;
      }

      for (const auto& propertyIt : properties) {
        if (pFeatureTable->Properties.FindByPredicate(
                [&propertyName = propertyIt.Key](
                    const FPropertyDescription& existingProperty) {
                  return existingProperty.Name == propertyName;
                })) {
          // We have already filled this property.
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
      FFeatureTextureDescription* pFeatureTexture =
          this->FeatureTextures.FindByPredicate(
              [&featureTextureName = featureTextureIt.Key](
                  const FFeatureTextureDescription& existingFeatureTexture) {
                return existingFeatureTexture.Name == featureTextureName;
              });

      if (!pFeatureTexture) {
        pFeatureTexture = &this->FeatureTextures.Emplace_GetRef();
        pFeatureTexture->Name = featureTextureIt.Key;
      }

      const TArray<FString>& propertyNames =
          UCesiumFeatureTextureBlueprintLibrary::GetPropertyKeys(
              featureTextureIt.Value);

      for (const FString& propertyName : propertyNames) {
        if (pFeatureTexture->Properties.FindByPredicate(
                [&propertyName](const FFeatureTexturePropertyDescription&
                                    existingProperty) {
                  return propertyName == existingProperty.Name;
                })) {
          // We have already filled this property.
          continue;
        }

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
            UCesiumFeatureTexturePropertyBlueprintLibrary::GetSwizzle(property);
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

// Seperate nodes into auto-generated and user-added. Collect the property
// result nodes.
static void ClassifyNodes(
    UMaterialFunctionMaterialLayer* Layer,
    TArray<UMaterialExpression*>& AutoGeneratedNodes,
    TArray<UMaterialExpression*>& UserAddedNodes,
    TArray<UMaterialExpressionCustom*>& ResultNodes) {

  for (UMaterialExpression* Node : Layer->FunctionExpressions) {
    // Check if this node is marked as autogenerated.
    if (Node->Desc.StartsWith(
            "AUTOGENERATED DO NOT EDIT",
            ESearchCase::Type::CaseSensitive)) {
      AutoGeneratedNodes.Add(Node);

      // The only auto-generated custom nodes are the property result nodes.
      UMaterialExpressionCustom* CustomNode =
          Cast<UMaterialExpressionCustom>(Node);
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
      FString Key =
          ResultNode->Description + PropertyOutput.OutputName.ToString();

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
  for (UMaterialExpression* AutoGeneratedNode : AutoGeneratedNodes) {
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
      FString Key =
          ResultNode->Description + PropertyOutput.OutputName.ToString();

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

  FString MaterialName = "ML_" + pTileset->GetFName().ToString() + "_Metadata";
  FString PackageBaseName = "/Game/";
  FString PackageName = PackageBaseName + MaterialName;

  UMaterialFunction* SelectTexCoordsFunction = LoadMaterialFunction(
      "/CesiumForUnreal/Materials/MaterialFunctions/CesiumSelectTexCoords.CesiumSelectTexCoords");
  if (!SelectTexCoordsFunction) {
    return;
  }

  bool Overwriting = false;
  if (this->TargetMaterialLayer) {
    // Overwriting an existing material layer.
    Overwriting = true;
    GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()
        ->CloseAllEditorsForAsset(this->TargetMaterialLayer);
  } else {
    UPackage* Package = CreatePackage(*PackageName);

    // Create an unreal material asset
    UMaterialFunctionMaterialLayerFactory* MaterialFactory =
        NewObject<UMaterialFunctionMaterialLayerFactory>();
    this->TargetMaterialLayer =
        (UMaterialFunctionMaterialLayer*)MaterialFactory->FactoryCreateNew(
            UMaterialFunctionMaterialLayer::StaticClass(),
            Package,
            *MaterialName,
            RF_Standalone | RF_Public | RF_Transactional,
            NULL,
            GWarn);
    FAssetRegistryModule::AssetCreated(this->TargetMaterialLayer);
    Package->FullyLoad();
    Package->SetDirtyFlag(true);
  }

  this->TargetMaterialLayer->PreEditChange(NULL);

  TMap<FString, TArray<FExpressionInput*>> ConnectionRemap;
  ClearAutoGeneratedNodes(this->TargetMaterialLayer, ConnectionRemap);

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
        NewObject<UMaterialExpressionCustom>(this->TargetMaterialLayer);
    FeatureTableLookup->Inputs.Reserve(featureTable.Properties.Num() + 2);
    FeatureTableLookup->Outputs.Reset(featureTable.Properties.Num() + 1);
    FeatureTableLookup->Outputs.Add(FExpressionOutput(TEXT("return")));
    FeatureTableLookup->bShowOutputNameOnPin = true;
    FeatureTableLookup->Description =
        "Resolve properties from " + featureTable.Name;
    AutoGeneratedNodes.Add(FeatureTableLookup);

    if (featureTable.AccessType == ECesiumFeatureTableAccessType::Texture) {
      UMaterialExpressionTextureObjectParameter* FeatureIdTexture =
          NewObject<UMaterialExpressionTextureObjectParameter>(
              this->TargetMaterialLayer);
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
          NewObject<UMaterialExpressionScalarParameter>(
              this->TargetMaterialLayer);
      TexCoordsIndex->ParameterName = FName("FIT_" + featureTable.Name + "_UV");
      TexCoordsIndex->DefaultValue = 0.0f;
      TexCoordsIndex->MaterialExpressionEditorX = NodeX;
      TexCoordsIndex->MaterialExpressionEditorY = NodeY;
      AutoGeneratedNodes.Add(TexCoordsIndex);

      NodeX += IncrX;

      UMaterialExpressionMaterialFunctionCall* SelectTexCoords =
          NewObject<UMaterialExpressionMaterialFunctionCall>(
              this->TargetMaterialLayer);
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
          "uint _czm_propertyIndex = asuint(FeatureIdTexture.Sample(FeatureIdTextureSampler, TexCoords)." +
          featureTable.Channel + ");\n";

      FeatureTableLookup->MaterialExpressionEditorX = NodeX;
      FeatureTableLookup->MaterialExpressionEditorY = NodeY;
    } else {
      // Create material for vertex attributes

      UMaterialExpressionScalarParameter* AttributeIndex =
          NewObject<UMaterialExpressionScalarParameter>(
              this->TargetMaterialLayer);
      AttributeIndex->ParameterName = FName("FA_" + featureTable.Name);
      AttributeIndex->DefaultValue = 0.0f;
      AttributeIndex->MaterialExpressionEditorX = NodeX;
      AttributeIndex->MaterialExpressionEditorY = NodeY;
      AutoGeneratedNodes.Add(AttributeIndex);

      NodeX += IncrX;

      UMaterialExpressionMaterialFunctionCall* SelectTexCoords =
          NewObject<UMaterialExpressionMaterialFunctionCall>(
              this->TargetMaterialLayer);
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

      FeatureTableLookup->Code =
          "uint _czm_propertyIndex = round(PropertyIndexUV.r);\n";

      FeatureTableLookup->MaterialExpressionEditorX = NodeX;
      FeatureTableLookup->MaterialExpressionEditorY = NodeY;
    }

    // Get the pixel dimensions of the first property, all the properties will
    // have the same dimensions since it is based on the feature count.
    if (featureTable.Properties.Num()) {
      const FPropertyDescription& property = featureTable.Properties[0];
      FString propertyArrayName = createHlslSafeName(property.Name) + "_array";

      FeatureTableLookup->Code += "uint _czm_width;\nuint _czm_height;\n";
      FeatureTableLookup->Code +=
          propertyArrayName + ".GetDimensions(_czm_width, _czm_height);\n";
      FeatureTableLookup->Code +=
          "uint _czm_pixelX = _czm_propertyIndex % _czm_width;\n";
      FeatureTableLookup->Code +=
          "uint _czm_pixelY = _czm_propertyIndex / _czm_width;\n";
    }

    NodeX = SectionLeft;
    NodeY += IncrY;

    FeatureTableLookup->AdditionalOutputs.Reserve(
        featureTable.Properties.Num());
    for (const FPropertyDescription& property : featureTable.Properties) {
      UMaterialExpressionTextureObjectParameter* PropertyArray =
          NewObject<UMaterialExpressionTextureObjectParameter>(
              this->TargetMaterialLayer);

      FString propertyName = createHlslSafeName(property.Name);

      PropertyArray->ParameterName =
          FName("FTB_" + featureTable.Name + "_" + propertyName);
      PropertyArray->MaterialExpressionEditorX = NodeX;
      PropertyArray->MaterialExpressionEditorY = NodeY;
      AutoGeneratedNodes.Add(PropertyArray);

      FCustomInput& PropertyInput = FeatureTableLookup->Inputs.Emplace_GetRef();
      FString propertyArrayName = propertyName + "_array";
      PropertyInput.InputName = FName(propertyArrayName);
      PropertyInput.Input.Expression = PropertyArray;

      FCustomOutput& PropertyOutput =
          FeatureTableLookup->AdditionalOutputs.Emplace_GetRef();
      PropertyOutput.OutputName = FName(propertyName);
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
          propertyArrayName + ".Load(int3(_czm_pixelX, _czm_pixelY, 0))." +
          swizzle + ");\n";

      NodeY += IncrY;
    }

    FeatureTableLookup->OutputType = ECustomMaterialOutputType::CMOT_Float1;

    FeatureTableLookup->Code +=
        "float _czm_propertyIndexF = _czm_propertyIndex;\n";
    FeatureTableLookup->Code += "return _czm_propertyIndexF;";

    NodeX = SectionLeft;
  }

  for (const FFeatureTextureDescription& featureTexture :
       this->FeatureTextures) {
    int32 SectionLeft = NodeX;
    int32 SectionTop = NodeY;

    UMaterialExpressionCustom* FeatureTextureLookup =
        NewObject<UMaterialExpressionCustom>(this->TargetMaterialLayer);
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
          NewObject<UMaterialExpressionTextureObjectParameter>(
              this->TargetMaterialLayer);

      FString propertyName = createHlslSafeName(property.Name);

      PropertyTexture->ParameterName =
          FName("FTX_" + featureTexture.Name + "_" + propertyName + "_TX");
      PropertyTexture->MaterialExpressionEditorX = NodeX;
      PropertyTexture->MaterialExpressionEditorY = NodeY;
      AutoGeneratedNodes.Add(PropertyTexture);

      FCustomInput& PropertyTextureInput =
          FeatureTextureLookup->Inputs.Emplace_GetRef();
      FString propertyTextureName = propertyName + "_TX";
      PropertyTextureInput.InputName = FName(propertyTextureName);
      PropertyTextureInput.Input.Expression = PropertyTexture;

      NodeY += IncrY;

      UMaterialExpressionScalarParameter* TexCoordsIndex =
          NewObject<UMaterialExpressionScalarParameter>(
              this->TargetMaterialLayer);
      TexCoordsIndex->ParameterName =
          FName("FTX_" + featureTexture.Name + "_" + propertyName + "_UV");
      TexCoordsIndex->DefaultValue = 0.0f;
      TexCoordsIndex->MaterialExpressionEditorX = NodeX;
      TexCoordsIndex->MaterialExpressionEditorY = NodeY;
      AutoGeneratedNodes.Add(TexCoordsIndex);

      NodeX += IncrX;

      UMaterialExpressionMaterialFunctionCall* SelectTexCoords =
          NewObject<UMaterialExpressionMaterialFunctionCall>(
              this->TargetMaterialLayer);
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
  for (UMaterialExpression* ExistingNode :
       this->TargetMaterialLayer->FunctionExpressions) {
    UMaterialExpressionFunctionInput* ExistingInputMaterial =
        Cast<UMaterialExpressionFunctionInput>(ExistingNode);
    if (ExistingInputMaterial) {
      InputMaterial = ExistingInputMaterial;
      break;
    }
  }

  if (!InputMaterial) {
    InputMaterial =
        NewObject<UMaterialExpressionFunctionInput>(this->TargetMaterialLayer);
    InputMaterial->InputType =
        EFunctionInputType::FunctionInput_MaterialAttributes;
    InputMaterial->bUsePreviewValueAsDefault = true;
    InputMaterial->MaterialExpressionEditorX = NodeX;
    InputMaterial->MaterialExpressionEditorY = NodeY;
    OneTimeGeneratedNodes.Add(InputMaterial);
  }

  NodeX += 4 * IncrX;

  UMaterialExpressionSetMaterialAttributes* SetMaterialAttributes = nullptr;
  for (UMaterialExpression* ExistingNode :
       this->TargetMaterialLayer->FunctionExpressions) {
    UMaterialExpressionSetMaterialAttributes* ExistingSetAttributes =
        Cast<UMaterialExpressionSetMaterialAttributes>(ExistingNode);
    if (ExistingSetAttributes) {
      SetMaterialAttributes = ExistingSetAttributes;
      break;
    }
  }

  if (!SetMaterialAttributes) {
    SetMaterialAttributes = NewObject<UMaterialExpressionSetMaterialAttributes>(
        this->TargetMaterialLayer);
    OneTimeGeneratedNodes.Add(SetMaterialAttributes);
  }

  SetMaterialAttributes->Inputs[0].Expression = InputMaterial;
  SetMaterialAttributes->MaterialExpressionEditorX = NodeX;
  SetMaterialAttributes->MaterialExpressionEditorY = NodeY;

  NodeX += IncrX;

  UMaterialExpressionFunctionOutput* OutputMaterial = nullptr;
  for (UMaterialExpression* ExistingNode :
       this->TargetMaterialLayer->FunctionExpressions) {
    UMaterialExpressionFunctionOutput* ExistingOutputMaterial =
        Cast<UMaterialExpressionFunctionOutput>(ExistingNode);
    if (ExistingOutputMaterial) {
      OutputMaterial = ExistingOutputMaterial;
      break;
    }
  }

  if (!OutputMaterial) {
    OutputMaterial =
        NewObject<UMaterialExpressionFunctionOutput>(this->TargetMaterialLayer);
    OneTimeGeneratedNodes.Add(OutputMaterial);
  }

  OutputMaterial->MaterialExpressionEditorX = NodeX;
  OutputMaterial->MaterialExpressionEditorY = NodeY;
  OutputMaterial->A = FMaterialAttributesInput();
  OutputMaterial->A.Expression = SetMaterialAttributes;

  for (UMaterialExpression* AutoGeneratedNode : AutoGeneratedNodes) {
    this->TargetMaterialLayer->FunctionExpressions.Add(AutoGeneratedNode);
    // Mark as auto-generated. If the material is regenerated, we will look for
    // this exact description to determine whether it was autogenerated.
    // Completely open to suggestions of how else to mark custom information on
    // these assets :)
    AutoGeneratedNode->Desc = "AUTOGENERATED DO NOT EDIT";
  }

  for (UMaterialExpression* OneTimeGeneratedNode : OneTimeGeneratedNodes) {
    this->TargetMaterialLayer->FunctionExpressions.Add(OneTimeGeneratedNode);
  }

  RemapUserConnections(this->TargetMaterialLayer, ConnectionRemap);

  // let the material update itself if necessary
  this->TargetMaterialLayer->PostEditChange();

  // make sure that any static meshes, etc using this material will stop using
  // the FMaterialResource of the original material, and will use the new
  // FMaterialResource created when we make a new UMaterial in place
  FGlobalComponentReregisterContext RecreateComponents;

  // If this is a new material open the content browser to the auto-generated
  // material.
  if (!Overwriting) {
    FContentBrowserModule* pContentBrowserModule =
        FModuleManager::Get().GetModulePtr<FContentBrowserModule>(
            "ContentBrowser");
    if (pContentBrowserModule) {
      TArray<UObject*> AssetsToHighlight;
      AssetsToHighlight.Add(this->TargetMaterialLayer);
      pContentBrowserModule->Get().SyncBrowserToAssets(AssetsToHighlight);
    }
  }

  // Open updated material in editor.
  if (GEditor) {
    UAssetEditorSubsystem* pAssetEditor =
        GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
    if (pAssetEditor) {
      pAssetEditor->OpenEditorForAsset(this->TargetMaterialLayer);
      IMaterialEditor* pMaterialEditor = static_cast<IMaterialEditor*>(
          pAssetEditor->FindEditorForAsset(this->TargetMaterialLayer, true));
      if (pMaterialEditor) {
        pMaterialEditor->UpdateMaterialAfterGraphChange();
      }
    }
  }
}

#endif // WITH_EDITOR
