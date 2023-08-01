// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumFeaturesMetadataComponent.h"
#include "Cesium3DTileset.h"
#include "CesiumEncodedFeaturesMetadata.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumMetadataConversions.h"
#include "CesiumModelMetadata.h"

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

using namespace CesiumEncodedFeaturesMetadata;

namespace {
FString GetNameForPropertyTable(const FCesiumPropertyTable& propertyTable) {
  FString propertyTableName =
      UCesiumPropertyTableBlueprintLibrary::GetPropertyTableName(propertyTable);

  if (propertyTableName.IsEmpty()) {
    // Substitute the name with the property table's class.
    propertyTableName = propertyTable.getClass();
  }

  return propertyTableName;
}

void AutoFillPropertyTableDescriptions(
    TArray<FCesiumPropertyTableDescription>& Descriptions,
    const FCesiumModelMetadata& ModelMetadata) {
  const TArray<FCesiumPropertyTable>& propertyTables =
      UCesiumModelMetadataBlueprintLibrary::GetPropertyTables(ModelMetadata);

  for (int32 i = 0; i < propertyTables.Num(); i++) {
    FCesiumPropertyTable propertyTable = propertyTables[i];
    FString propertyTableName = GetNameForPropertyTable(propertyTable);

    FCesiumPropertyTableDescription* pDescription =
        Descriptions.FindByPredicate(
            [&name = propertyTableName](
                const FCesiumPropertyTableDescription& existingPropertyTable) {
              return existingPropertyTable.Name == name;
            });

    if (!pDescription) {
      pDescription = &Descriptions.Emplace_GetRef();
      pDescription->Name = propertyTableName;
    }

    const TMap<FString, FCesiumPropertyTableProperty>& properties =
        UCesiumPropertyTableBlueprintLibrary::GetProperties(propertyTable);
    for (const auto& propertyIt : properties) {
      if (pDescription->Properties.FindByPredicate(
              [&propertyName = propertyIt.Key](
                  const FCesiumPropertyTablePropertyDescription&
                      existingProperty) {
                return existingProperty.Name == propertyName;
              })) {
        // We have already accounted for this property; skip.
        continue;
      }

      FCesiumMetadataValueType valueType =
          UCesiumPropertyTablePropertyBlueprintLibrary::GetValueType(
              propertyIt.Value);
      ECesiumMetadataType type = valueType.Type;
      ECesiumMetadataComponentType componentType = valueType.ComponentType;

      ECesiumEncodedMetadataGpuType gpuType =
          CesiumMetadataValueTypeToEncodedMetadataGpuType(valueType);
      if (gpuType == ECesiumEncodedMetadataGpuType::None) {
        continue;
      }

      if (valueType.bIsArray) {
        UCesiumPropertyTablePropertyBlueprintLibrary::GetArraySize(
            propertyIt.Value);
      }

      FCesiumPropertyTablePropertyDescription& property =
          pDescription->Properties.Emplace_GetRef();
      property.Name = propertyIt.Key;

      switch (type) {
      case ECesiumMetadataType::Vec2:
        property.Type = ECesiumEncodedPropertyType::Vec2;
        break;
      case ECesiumMetadataType::Vec3:
        property.Type = ECesiumEncodedPropertyType::Vec3;
        break;
      case ECesiumMetadataType::Vec4:
        property.Type = ECesiumEncodedPropertyType::Vec4;
        break;
      default:
        property.Type = ECesiumEncodedPropertyType::Scalar;
      };

      if (gpuType == ECesiumEncodedMetadataGpuType::Uint8) {
        property.ComponentType = ECesiumEncodedPropertyComponentType::Uint8;
      } else /*if (gpuType == float)*/ {
        property.ComponentType = ECesiumEncodedPropertyComponentType::Float;
      }

      property.Normalized =
          UCesiumPropertyTablePropertyBlueprintLibrary::IsNormalized(
              propertyIt.Value);
    }
  }
}

void AutoFillFeatureIdSetDescriptions(
    TArray<FCesiumFeatureIdSetDescription>& Descriptions,
    const FCesiumPrimitiveFeatures& Features,
    const TArray<FCesiumPropertyTable>& PropertyTables) {
  const TArray<FCesiumFeatureIdSet> featureIDSets =
      UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(Features);
  int32 featureIDTextureCounter = 0;

  for (const FCesiumFeatureIdSet& featureIDSet : featureIDSets) {
    ECesiumFeatureIdSetType type =
        UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDSetType(featureIDSet);
    int64 count =
        UCesiumFeatureIdSetBlueprintLibrary::GetFeatureCount(featureIDSet);
    if (type == ECesiumFeatureIdSetType::None || count == 0) {
      // Empty or invalid feature ID set. Skip.
      continue;
    }

    FString featureIDSetName =
        getNameForFeatureIDSet(featureIDSet, featureIDTextureCounter);
    FCesiumFeatureIdSetDescription* pDescription = Descriptions.FindByPredicate(
        [&name = featureIDSetName](
            const FCesiumFeatureIdSetDescription& existingFeatureIDSet) {
          return existingFeatureIDSet.Name == name;
        });

    if (pDescription) {
      // We have already accounted for a feature ID set of this name; skip.
      continue;
    }

    pDescription = &Descriptions.Emplace_GetRef();
    pDescription->Name = featureIDSetName;
    pDescription->Type = type;

    const int64 propertyTableIndex =
        UCesiumFeatureIdSetBlueprintLibrary::GetPropertyTableIndex(
            featureIDSet);

    if (propertyTableIndex >= 0 && propertyTableIndex < PropertyTables.Num()) {
      const FCesiumPropertyTable& propertyTable =
          PropertyTables[propertyTableIndex];
      pDescription->PropertyTableName = GetNameForPropertyTable(propertyTable);
    }
  }
}

} // namespace

void UCesiumFeaturesMetadataComponent::AutoFill() {
  const ACesium3DTileset* pOwner = this->GetOwner<ACesium3DTileset>();
  if (!pOwner) {
    return;
  }

  // This assumes that the property tables are the same across all models in the
  // tileset, and that they all have the same schema.
  for (const UActorComponent* pComponent : pOwner->GetComponents()) {
    const UCesiumGltfComponent* pGltf = Cast<UCesiumGltfComponent>(pComponent);
    if (!pGltf) {
      continue;
    }

    const FCesiumModelMetadata& modelMetadata = pGltf->Metadata;
    AutoFillPropertyTableDescriptions(this->PropertyTables, modelMetadata);

    TArray<USceneComponent*> childComponents;
    pGltf->GetChildrenComponents(false, childComponents);

    for (const USceneComponent* pChildComponent : childComponents) {
      const UCesiumGltfPrimitiveComponent* pGltfPrimitive =
          Cast<UCesiumGltfPrimitiveComponent>(pChildComponent);
      if (!pGltfPrimitive) {
        continue;
      }

      const FCesiumPrimitiveFeatures& primitiveFeatures =
          pGltfPrimitive->Features;
      const TArray<FCesiumPropertyTable>& propertyTables =
          UCesiumModelMetadataBlueprintLibrary::GetPropertyTables(
              modelMetadata);
      AutoFillFeatureIdSetDescriptions(
          this->FeatureIdSets,
          primitiveFeatures,
          propertyTables);
    }

    // const TMap<FString, FCesiumFeatureTexture>& featureTextures =
    //    UCesiumMetadataModelBlueprintLibrary::GetFeatureTextures(model);

    // for (const auto& featureTextureIt : featureTextures) {
    //  FFeatureTextureDescription* pFeatureTexture =
    //      this->FeatureTextures.FindByPredicate(
    //          [&featureTextureName = featureTextureIt.Key](
    //              const FFeatureTextureDescription& existingFeatureTexture) {
    //            return existingFeatureTexture.Name == featureTextureName;
    //          });

    //  if (!pFeatureTexture) {
    //    pFeatureTexture = &this->FeatureTextures.Emplace_GetRef();
    //    pFeatureTexture->Name = featureTextureIt.Key;
    //  }

    //  const TArray<FString>& propertyNames =
    //      UCesiumFeatureTextureBlueprintLibrary::GetPropertyKeys(
    //          featureTextureIt.Value);

    //  for (const FString& propertyName : propertyNames) {
    //    if (pFeatureTexture->Properties.FindByPredicate(
    //            [&propertyName](const FFeatureTexturePropertyDescription&
    //                                existingProperty) {
    //              return propertyName == existingProperty.Name;
    //            })) {
    //      // We have already filled this property.
    //      continue;
    //    }

    //    FCesiumPropertyTextureProperty property =
    //        UCesiumPropertyTextureBlueprintLibrary::FindProperty(
    //            featureTextureIt.Value,
    //            propertyName);
    //    FFeatureTexturePropertyDescription& propertyDescription =
    //        pFeatureTexture->Properties.Emplace_GetRef();
    //    propertyDescription.Name = propertyName;
    //    propertyDescription.Normalized =
    //        UCesiumPropertyTexturePropertyBlueprintLibrary::IsNormalized(
    //            property);

    //    switch (
    //        UCesiumPropertyTexturePropertyBlueprintLibrary::GetComponentCount(
    //            property)) {
    //    case 2:
    //      propertyDescription.Type = ECesiumPropertyType::Vec2;
    //      break;
    //    case 3:
    //      propertyDescription.Type = ECesiumPropertyType::Vec3;
    //      break;
    //    case 4:
    //      propertyDescription.Type = ECesiumPropertyType::Vec4;
    //      break;
    //    // case 1:
    //    default:
    //      propertyDescription.Type = ECesiumPropertyType::Scalar;
    //    }

    //    propertyDescription.Swizzle =
    //        UCesiumFeatureTexturePropertyBlueprintLibrary::GetSwizzle(property);
    //  }
    //}
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

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
  for (UMaterialExpression* Node : Layer->FunctionExpressions) {
#else
  for (const TObjectPtr<UMaterialExpression>& Node :
       Layer->GetExpressionCollection().Expressions) {
#endif
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
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
    Layer->FunctionExpressions.Remove(AutoGeneratedNode);
#else
    Layer->GetExpressionCollection().RemoveExpression(AutoGeneratedNode);
#endif
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

static const int32 IncrX = 400;
static const int32 IncrY = 200;

namespace {
void GenerateNodesForFeatureIdSets(
    const TArray<FCesiumFeatureIdSetDescription>& Descriptions,
    TArray<UMaterialExpression*>& AutoGeneratedNodes,
    UMaterialFunctionMaterialLayer* TargetMaterialLayer,
    UMaterialFunction* GetFeatureIdsFromAttributeFunction,
    int32& NodeX,
    int32& NodeY) {

  for (const FCesiumFeatureIdSetDescription& featureIdSet : Descriptions) {
    if (featureIdSet.Type == ECesiumFeatureIdSetType::None) {
      continue;
    }

    if (featureIdSet.Type == ECesiumFeatureIdSetType::Texture) {
      //UMaterialExpressionTextureObjectParameter* FeatureIdTexture =
      //    NewObject<UMaterialExpressionTextureObjectParameter>(
      //       TargetMaterialLayer);
      //FeatureIdTexture->ParameterName =
      //    FName("FIT_" + featureTable.Name + "_TX");
      //FeatureIdTexture->MaterialExpressionEditorX = NodeX;
      //FeatureIdTexture->MaterialExpressionEditorY = NodeY;
      //AutoGeneratedNodes.Add(FeatureIdTexture);

      //FCustomInput& FeatureIdTextureInput = FeatureTableLookup->Inputs[0];
      //FeatureIdTextureInput.InputName = "FeatureIdTexture";
      //FeatureIdTextureInput.Input.Expression = FeatureIdTexture;

      //NodeY += IncrY;

      //UMaterialExpressionScalarParameter* TexCoordsIndex =
      //    NewObject<UMaterialExpressionScalarParameter>(
      //        this->TargetMaterialLayer);
      //TexCoordsIndex->ParameterName = FName("FIT_" + featureTable.Name + "_UV");
      //TexCoordsIndex->DefaultValue = 0.0f;
      //TexCoordsIndex->MaterialExpressionEditorX = NodeX;
      //TexCoordsIndex->MaterialExpressionEditorY = NodeY;
      //AutoGeneratedNodes.Add(TexCoordsIndex);

      //NodeX += IncrX;

      //UMaterialExpressionMaterialFunctionCall* SelectTexCoords =
      //    NewObject<UMaterialExpressionMaterialFunctionCall>(
      //        this->TargetMaterialLayer);
      //SelectTexCoords->MaterialFunction = SelectTexCoordsFunction;
      //SelectTexCoords->MaterialExpressionEditorX = NodeX;
      //SelectTexCoords->MaterialExpressionEditorY = NodeY;

      //SelectTexCoordsFunction->GetInputsAndOutputs(
      //    SelectTexCoords->FunctionInputs,
      //    SelectTexCoords->FunctionOutputs);
      //SelectTexCoords->FunctionInputs[0].Input.Expression = TexCoordsIndex;
      //AutoGeneratedNodes.Add(SelectTexCoords);

      //FCustomInput& TexCoordsInput =
      //    FeatureTableLookup->Inputs.Emplace_GetRef();
      //TexCoordsInput.InputName = FName("TexCoords");
      //TexCoordsInput.Input.Expression = SelectTexCoords;

      //NodeX += IncrX;

      //// TODO: Should the channel mask be determined dynamically instead of
      //// at editor-time like it is now?
      //FeatureTableLookup->Code = "uint _czm_propertyIndex =
      //    asuint(FeatureIdTexture.Sample(FeatureIdTextureSampler, TexCoords)
      //               ." + featureTable.Channel + ");
    } else {
      // Handle implicit feature IDs the same as feature ID attributes
      UMaterialExpressionScalarParameter* TextureCoordinateIndex =
          NewObject<UMaterialExpressionScalarParameter>(TargetMaterialLayer);
      TextureCoordinateIndex->ParameterName = FName(featureIdSet.Name);
      TextureCoordinateIndex->DefaultValue = 0.0f;
      TextureCoordinateIndex->MaterialExpressionEditorX = NodeX;
      TextureCoordinateIndex->MaterialExpressionEditorY = NodeY;
      AutoGeneratedNodes.Add(TextureCoordinateIndex);

      NodeX += IncrX;

      UMaterialExpressionMaterialFunctionCall* GetFeatureIdsFromAttribute =
          NewObject<UMaterialExpressionMaterialFunctionCall>(
              TargetMaterialLayer);
      GetFeatureIdsFromAttribute->MaterialFunction =
          GetFeatureIdsFromAttributeFunction;
      GetFeatureIdsFromAttribute->MaterialExpressionEditorX = NodeX;
      GetFeatureIdsFromAttribute->MaterialExpressionEditorY = NodeY;

      GetFeatureIdsFromAttributeFunction->GetInputsAndOutputs(
          GetFeatureIdsFromAttribute->FunctionInputs,
          GetFeatureIdsFromAttribute->FunctionOutputs);
      GetFeatureIdsFromAttribute->FunctionInputs[0].Input.Expression =
          TextureCoordinateIndex;
      AutoGeneratedNodes.Add(GetFeatureIdsFromAttribute);
    }
  }
}

void GenerateNodesForPropertyTables(
    const TArray<FCesiumPropertyTableDescription>& Descriptions,
    TArray<UMaterialExpression*>& AutoGeneratedNodes,
    UMaterialFunctionMaterialLayer* TargetMaterialLayer,
    int32& NodeX,
    int32& NodeY) {
  for (const FCesiumPropertyTableDescription& propertyTable : Descriptions) {
    int32 SectionLeft = NodeX;
    int32 SectionTop = NodeY;

    UMaterialExpressionCustom* GetPropertyValuesFunction =
        NewObject<UMaterialExpressionCustom>(TargetMaterialLayer);
    GetPropertyValuesFunction->Inputs.Reserve(
        propertyTable.Properties.Num() + 2);
    GetPropertyValuesFunction->Outputs.Reset(
        propertyTable.Properties.Num() + 1);
    GetPropertyValuesFunction->Outputs.Add(FExpressionOutput(TEXT("return")));
    GetPropertyValuesFunction->bShowOutputNameOnPin = true;
    GetPropertyValuesFunction->Description =
        "Get property values from " + propertyTable.Name;
    AutoGeneratedNodes.Add(GetPropertyValuesFunction);

    // FCustomInput& TexCoordsInput = FeatureTableLookup->Inputs[0];
    // TexCoordsInput.InputName = FName("PropertyIndexUV");
    // TexCoordsInput.Input.Expression = SelectTexCoords;

    // NodeX += IncrX;

    // FeatureTableLookup->Code =
    //    "uint _czm_propertyIndex = round(PropertyIndexUV.r);\n";

    // FeatureTableLookup->MaterialExpressionEditorX = NodeX;
    // FeatureTableLookup->MaterialExpressionEditorY = NodeY;
    //

    //  // Get the pixel dimensions of the first property, all the properties
    //  // will
    //  // have the same dimensions since it is based on the feature count.
    //  if (propertyTable.Properties.Num()) {
    //    const FCesiumPropertyTablePropertyDescription& property =
    //        propertyTable.Properties[0];
    //    FString propertyArrayName =
    //        CesiumEncodedFeaturesMetadata::createHlslSafeName(property.Name) +
    //        "_array";

    //    GetPropertyValuesFunction->Code += "uint _czm_width;\nuint
    //        _czm_height;
    //    \n "; GetPropertyValuesFunction->Code +=
    //        propertyArrayName +
    //        ".GetDimensions(_czm_width, _czm_height);\n";
    //    GetPropertyValuesFunction->Code +=
    //        "uint _czm_pixelX = _czm_propertyIndex % _czm_width;\n";
    //    GetPropertyValuesFunction->Code +=
    //        "uint _czm_pixelY = _czm_propertyIndex / _czm_width;\n";
    //  }

    //  NodeX = SectionLeft;
    //  NodeY += IncrY;

    //  GetPropertyValuesFunction->AdditionalOutputs.Reserve(
    //      propertyTable.Properties.Num());
    //  for (const FCesiumPropertyTablePropertyDescription& property :
    //       propertyTable.Properties) {
    //    UMaterialExpressionTextureObjectParameter* PropertyArray =
    //        NewObject<UMaterialExpressionTextureObjectParameter>(
    //            TargetMaterialLayer);
    //    PropertyArray->ParameterName =
    //        FName("FTB_" + propertyTable.Name + "_" + property.Name);
    //    PropertyArray->MaterialExpressionEditorX = NodeX;
    //    PropertyArray->MaterialExpressionEditorY = NodeY;
    //    AutoGeneratedNodes.Add(PropertyArray);

    //    FString propertyName = createHlslSafeName(property.Name);

    //    FCustomInput& PropertyInput =
    //    FeatureTableLookup->Inputs.Emplace_GetRef(); FString propertyArrayName
    //    = propertyName + "_array"; PropertyInput.InputName =
    //    FName(propertyArrayName); PropertyInput.Input.Expression =
    //    PropertyArray;

    //    FCustomOutput& PropertyOutput =
    //        FeatureTableLookup->AdditionalOutputs.Emplace_GetRef();
    //    PropertyOutput.OutputName = FName(propertyName);
    //    FeatureTableLookup->Outputs.Add(
    //        FExpressionOutput(PropertyOutput.OutputName));

    //    FString swizzle = "";
    //    switch (property.Type) {
    //    case ECesiumPropertyType::Vec2:
    //      PropertyOutput.OutputType = ECustomMaterialOutputType::CMOT_Float2;
    //      swizzle = "rg";
    //      break;
    //    case ECesiumPropertyType::Vec3:
    //      PropertyOutput.OutputType = ECustomMaterialOutputType::CMOT_Float3;
    //      swizzle = "rgb";
    //      break;
    //    case ECesiumPropertyType::Vec4:
    //      PropertyOutput.OutputType = ECustomMaterialOutputType::CMOT_Float4;
    //      swizzle = "rgba";
    //      break;
    //    // case ECesiumPropertyType::Scalar:
    //    default:
    //      PropertyOutput.OutputType = ECustomMaterialOutputType::CMOT_Float1;
    //      swizzle = "r";
    //    };

    //    FString componentTypeInterpretation =
    //        property.ComponentType == ECesiumPropertyComponentType::Float
    //            ? "asfloat"
    //            : "asuint";

    //    FeatureTableLookup->Code +=
    //        propertyName + " = " + componentTypeInterpretation + "(" +
    //        propertyArrayName + ".Load(int3(_czm_pixelX, _czm_pixelY, 0))." +
    //        swizzle + ");\n";

    //    NodeY += IncrY;
    //  }

    //  FeatureTableLookup->OutputType = ECustomMaterialOutputType::CMOT_Float1;

    //  FeatureTableLookup->Code +=
    //      "float _czm_propertyIndexF = _czm_propertyIndex;\n";
    //  FeatureTableLookup->Code += "return _czm_propertyIndexF;";

    //  NodeX = SectionLeft;
    //}
  }
}

void GenerateNodesForPropertyTextures(
    // const TArray<FCesiumPropertyTableDescription>& Descriptions,
    TArray<UMaterialExpression*>& AutoGeneratedNodes,
    UMaterialFunctionMaterialLayer* TargetMaterialLayer,
    int32& NodeX,
    int32& NodeY) {
  //  for (const FFeatureTextureDescription& featureTexture :
  //       this->FeatureTextures) {
  //    int32 SectionLeft = NodeX;
  //    int32 SectionTop = NodeY;
  //
  //    UMaterialExpressionCustom* FeatureTextureLookup =
  //        NewObject<UMaterialExpressionCustom>(this->TargetMaterialLayer);
  //    FeatureTextureLookup->Inputs.Reset(2 * featureTexture.Properties.Num());
  //    FeatureTextureLookup->Outputs.Reset(featureTexture.Properties.Num() +
  //    1);
  //    FeatureTextureLookup->Outputs.Add(FExpressionOutput(TEXT("return")));
  //    FeatureTextureLookup->bShowOutputNameOnPin = true;
  //    FeatureTextureLookup->Code = "";
  //    FeatureTextureLookup->Description =
  //        "Resolve properties from " + featureTexture.Name;
  //    FeatureTextureLookup->MaterialExpressionEditorX = NodeX + 2 * IncrX;
  //    FeatureTextureLookup->MaterialExpressionEditorY = NodeY;
  //    AutoGeneratedNodes.Add(FeatureTextureLookup);
  //
  //    for (const FFeatureTexturePropertyDescription& property :
  //         featureTexture.Properties) {
  //      UMaterialExpressionTextureObjectParameter* PropertyTexture =
  //          NewObject<UMaterialExpressionTextureObjectParameter>(
  //              this->TargetMaterialLayer);
  //      PropertyTexture->ParameterName =
  //          FName("FTX_" + featureTexture.Name + "_" + property.Name + "_TX");
  //      PropertyTexture->MaterialExpressionEditorX = NodeX;
  //      PropertyTexture->MaterialExpressionEditorY = NodeY;
  //      AutoGeneratedNodes.Add(PropertyTexture);
  //
  //      FString propertyName = createHlslSafeName(property.Name);
  //
  //      FCustomInput& PropertyTextureInput =
  //          FeatureTextureLookup->Inputs.Emplace_GetRef();
  //      FString propertyTextureName = propertyName + "_TX";
  //      PropertyTextureInput.InputName = FName(propertyTextureName);
  //      PropertyTextureInput.Input.Expression = PropertyTexture;
  //
  //      NodeY += IncrY;
  //
  //      UMaterialExpressionScalarParameter* TexCoordsIndex =
  //          NewObject<UMaterialExpressionScalarParameter>(
  //              this->TargetMaterialLayer);
  //      TexCoordsIndex->ParameterName =
  //          FName("FTX_" + featureTexture.Name + "_" + property.Name + "_UV");
  //      TexCoordsIndex->DefaultValue = 0.0f;
  //      TexCoordsIndex->MaterialExpressionEditorX = NodeX;
  //      TexCoordsIndex->MaterialExpressionEditorY = NodeY;
  //      AutoGeneratedNodes.Add(TexCoordsIndex);
  //
  //      NodeX += IncrX;
  //
  //      UMaterialExpressionMaterialFunctionCall* SelectTexCoords =
  //          NewObject<UMaterialExpressionMaterialFunctionCall>(
  //              this->TargetMaterialLayer);
  //      SelectTexCoords->MaterialFunction = SelectTexCoordsFunction;
  //      SelectTexCoords->MaterialExpressionEditorX = NodeX;
  //      SelectTexCoords->MaterialExpressionEditorY = NodeY;
  //
  //      SelectTexCoordsFunction->GetInputsAndOutputs(
  //          SelectTexCoords->FunctionInputs,
  //          SelectTexCoords->FunctionOutputs);
  //      SelectTexCoords->FunctionInputs[0].Input.Expression = TexCoordsIndex;
  //      AutoGeneratedNodes.Add(SelectTexCoords);
  //
  //      FCustomInput& TexCoordsInput =
  //          FeatureTextureLookup->Inputs.Emplace_GetRef();
  //      FString propertyUvName = propertyName + "_UV";
  //      TexCoordsInput.InputName = FName(propertyUvName);
  //      TexCoordsInput.Input.Expression = SelectTexCoords;
  //
  //      FCustomOutput& PropertyOutput =
  //          FeatureTextureLookup->AdditionalOutputs.Emplace_GetRef();
  //      PropertyOutput.OutputName = FName(propertyName);
  //      FeatureTextureLookup->Outputs.Add(
  //          FExpressionOutput(PropertyOutput.OutputName));
  //
  //      // Either the property is normalized or it is coerced into float.
  //      Either
  //      // way, the outputs will be float type.
  //      switch (property.Type) {
  //      case ECesiumPropertyType::Vec2:
  //        PropertyOutput.OutputType = ECustomMaterialOutputType::CMOT_Float2;
  //        break;
  //      case ECesiumPropertyType::Vec3:
  //        PropertyOutput.OutputType = ECustomMaterialOutputType::CMOT_Float3;
  //        break;
  //      case ECesiumPropertyType::Vec4:
  //        PropertyOutput.OutputType = ECustomMaterialOutputType::CMOT_Float4;
  //        break;
  //      // case ECesiumPropertyType::Scalar:
  //      default:
  //        PropertyOutput.OutputType = ECustomMaterialOutputType::CMOT_Float1;
  //      };
  //
  //      // TODO: should dynamic channel offsets be used instead of swizzle
  //      string
  //      // determined at editor time? E.g. can swizzles be different for the
  //      same
  //      // property texture on different tiles?
  //      FeatureTextureLookup->Code +=
  //          propertyName + " = " +
  //          (property.Normalized ? "asfloat(" : "asuint(") +
  //          propertyTextureName +
  //          ".Sample(" + propertyTextureName + "Sampler, " + propertyUvName +
  //          ")." + property.Swizzle + ");\n";
  //
  //      NodeY += IncrY;
  //    }
  //
  //    FeatureTextureLookup->OutputType =
  //    ECustomMaterialOutputType::CMOT_Float1; FeatureTextureLookup->Code +=
  //    "return 0.0f;";
  //
  //    NodeX = SectionLeft;
  //  }
  //
}
} // namespace

void UCesiumFeaturesMetadataComponent::GenerateMaterial() {
  ACesium3DTileset* pTileset = Cast<ACesium3DTileset>(this->GetOwner());

  if (!pTileset) {
    return;
  }

  FString MaterialName =
      "ML_" + pTileset->GetFName().ToString() + "_FeaturesMetadata";
  FString PackageBaseName = "/Game/";
  FString PackageName = PackageBaseName + MaterialName;

  UMaterialFunction* SelectTexCoordsFunction = LoadMaterialFunction(
      "/CesiumForUnreal/Materials/MaterialFunctions/CesiumSelectTexCoords.CesiumSelectTexCoords");
  UMaterialFunction* GetFeatureIdsFromAttributeFunction = LoadMaterialFunction(
      "/CesiumForUnreal/Materials/MaterialFunctions/CesiumGetFeatureIdsFromAttribute.CesiumGetFeatureIdsFromAttribute");

  if (!SelectTexCoordsFunction || !GetFeatureIdsFromAttributeFunction) {
    return;
  }

  // TODO: make functions for feature ID texture

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
            RF_Standalone | RF_Transactional,
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

  int32 NodeX = 0;
  int32 NodeY = 0;

  GenerateNodesForFeatureIdSets(
      this->FeatureIdSets,
      AutoGeneratedNodes,
      TargetMaterialLayer,
      GetFeatureIdsFromAttributeFunction,
      NodeX,
      NodeY);

  GenerateNodesForPropertyTables(
      this->PropertyTables,
      AutoGeneratedNodes,
      TargetMaterialLayer,
      NodeX,
      NodeY);

  // GenerateNodesForPropertyTextures

  //  NodeY = -IncrY;

  UMaterialExpressionFunctionInput* InputMaterial = nullptr;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
  for (UMaterialExpression* ExistingNode :
       this->TargetMaterialLayer->FunctionExpressions) {
#else
  for (const TObjectPtr<UMaterialExpression>& ExistingNode :
       this->TargetMaterialLayer->GetExpressionCollection().Expressions) {
#endif
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
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
  for (UMaterialExpression* ExistingNode :
       this->TargetMaterialLayer->FunctionExpressions) {
#else
  for (const TObjectPtr<UMaterialExpression>& ExistingNode :
       this->TargetMaterialLayer->GetExpressionCollection().Expressions) {
#endif
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
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
  for (UMaterialExpression* ExistingNode :
       this->TargetMaterialLayer->FunctionExpressions) {
#else
  for (const TObjectPtr<UMaterialExpression>& ExistingNode :
       this->TargetMaterialLayer->GetExpressionCollection().Expressions) {
#endif
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
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
    this->TargetMaterialLayer->FunctionExpressions.Add(AutoGeneratedNode);
#else
    this->TargetMaterialLayer->GetExpressionCollection().AddExpression(
        AutoGeneratedNode);
#endif

    // Mark as auto-generated. If the material is regenerated, we will look for
    // this exact description to determine whether it was autogenerated.
    AutoGeneratedNode->Desc = "AUTOGENERATED DO NOT EDIT";
  }

  for (UMaterialExpression* OneTimeGeneratedNode : OneTimeGeneratedNodes) {
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
    this->TargetMaterialLayer->FunctionExpressions.Add(OneTimeGeneratedNode);
#else
    this->TargetMaterialLayer->GetExpressionCollection().AddExpression(
        OneTimeGeneratedNode);
#endif
  }

  RemapUserConnections(this->TargetMaterialLayer, ConnectionRemap);

  // let the material update itself if necessary
  this->TargetMaterialLayer->PostEditChange();

  // make sure that any static meshes, etc using this material will stop
  // using the FMaterialResource of the original material, and will use the new
  // FMaterialResource created when we make a new UMaterial in place
  FGlobalComponentReregisterContext RecreateComponents;

  // If this is a new material open the content browser to the
  // auto-generated material.
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
