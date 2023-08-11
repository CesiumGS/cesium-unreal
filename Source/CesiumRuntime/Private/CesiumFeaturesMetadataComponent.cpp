// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumFeaturesMetadataComponent.h"
#include "Cesium3DTileset.h"
#include "CesiumEncodedFeaturesMetadata.h"
#include "CesiumEncodedMetadataConversions.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumMetadataConversions.h"
#include "CesiumModelMetadata.h"
#include "CesiumRuntime.h"

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
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/Package.h"

extern UNREALED_API class UEditorEngine* GEditor;
#endif

using namespace CesiumEncodedFeaturesMetadata;

namespace {
void AutoFillPropertyTableDescriptions(
    TArray<FCesiumPropertyTableDescription>& Descriptions,
    const FCesiumModelMetadata& ModelMetadata) {
  const TArray<FCesiumPropertyTable>& propertyTables =
      UCesiumModelMetadataBlueprintLibrary::GetPropertyTables(ModelMetadata);

  for (int32 i = 0; i < propertyTables.Num(); i++) {
    FCesiumPropertyTable propertyTable = propertyTables[i];
    FString propertyTableName = getNameForPropertyTable(propertyTable);

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

      FCesiumPropertyTablePropertyDescription& property =
          pDescription->Properties.Emplace_GetRef();
      property.Name = propertyIt.Key;

      const FCesiumMetadataValueType ValueType =
          UCesiumPropertyTablePropertyBlueprintLibrary::GetValueType(
              propertyIt.Value);
      property.PropertyDetails.SetValueType(ValueType);
      property.PropertyDetails.ArraySize =
          UCesiumPropertyTablePropertyBlueprintLibrary::GetArraySize(
              propertyIt.Value);
      property.PropertyDetails.bIsNormalized =
          UCesiumPropertyTablePropertyBlueprintLibrary::IsNormalized(
              propertyIt.Value);

      property.EncodingDetails = CesiumMetadataPropertyDetailsToEncodingDetails(
          property.PropertyDetails);
    }
  }
}

void AutoFillPropertyTextureDescriptions() {
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
      pDescription->PropertyTableName = getNameForPropertyTable(propertyTable);
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

namespace {
struct MaterialNodeClassification {
  TArray<UMaterialExpression*> AutoGeneratedNodes;
  TArray<UMaterialExpressionMaterialFunctionCall*> GetFeatureIdNodes;
  TArray<UMaterialExpressionCustom*> GetPropertyValueNodes;
  TArray<UMaterialExpression*> UserAddedNodes;
};
} // namespace

// Seperate nodes into auto-generated and user-added. Collect the property
// result nodes.
static void ClassifyNodes(
    UMaterialFunctionMaterialLayer* Layer,
    MaterialNodeClassification& Classification,
    UMaterialFunction* GetFeatureIdsFromAttributeFunction,
    UMaterialFunction* GetFeatureIdsFromTextureFunction) {
#if ENGINE_MINOR_VERSION == 0
  for (UMaterialExpression* Node : Layer->FunctionExpressions) {
#else
  for (const TObjectPtr<UMaterialExpression>& Node :
       Layer->GetExpressionCollection().Expressions) {
#endif
    // Check if this node is marked as autogenerated.
    if (Node->Desc.StartsWith(
            "AUTOGENERATED DO NOT EDIT",
            ESearchCase::Type::CaseSensitive)) {
      Classification.AutoGeneratedNodes.Add(Node);

      // The only auto-generated custom nodes are the property result nodes
      // (i.e., nodes named "Get Property Values From ___").
      UMaterialExpressionCustom* CustomNode =
          Cast<UMaterialExpressionCustom>(Node);
      if (CustomNode) {
        Classification.GetPropertyValueNodes.Add(CustomNode);
        continue;
      }

      UMaterialExpressionMaterialFunctionCall* FunctionCallNode =
          Cast<UMaterialExpressionMaterialFunctionCall>(Node);
      if (!FunctionCallNode)
        continue;

      const FName& name = FunctionCallNode->MaterialFunction->GetFName();
      if (name == GetFeatureIdsFromAttributeFunction->GetFName() ||
          name == GetFeatureIdsFromTextureFunction->GetFName()) {
        Classification.GetFeatureIdNodes.Add(FunctionCallNode);
      }
    } else {
      Classification.UserAddedNodes.Add(Node);
    }
  }
}

static void ClearAutoGeneratedNodes(
    UMaterialFunctionMaterialLayer* Layer,
    TMap<FString, TArray<FExpressionInput*>>& ConnectionRemap,
    UMaterialFunction* GetFeatureIdsFromAttributeFunction,
    UMaterialFunction* GetFeatureIdsFromTextureFunction) {

  MaterialNodeClassification Classification;
  ClassifyNodes(
      Layer,
      Classification,
      GetFeatureIdsFromAttributeFunction,
      GetFeatureIdsFromTextureFunction);

  // Determine which user-added connections to remap when regenerating the
  // feature ID retrieval nodes.
  for (const UMaterialExpressionMaterialFunctionCall* GetFeatureIdNode :
       Classification.GetFeatureIdNodes) {
    if (!GetFeatureIdNode->Outputs.Num()) {
      continue;
    }

    const auto Inputs = GetFeatureIdNode->FunctionInputs;

    if (!Inputs.Num()) {
      // Should not happen, but just in case, this node would be invalid. Break
      // any user-made connections to this node and don't attempt to remap it.
      for (UMaterialExpression* UserNode : Classification.UserAddedNodes) {
        for (FExpressionInput* Input : UserNode->GetInputs()) {
          if (Input->Expression == GetFeatureIdNode &&
              Input->OutputIndex == 0) {
            Input->Expression = nullptr;
          }
        }
      }
      continue;
    }

    // It's not as easy to distinguish the material function calls from each
    // other. Try using the name of the first valid input (the texcoord index
    // name), which should be different for each feature ID set.
    const auto Parameter =
        Cast<UMaterialExpressionParameter>(Inputs[0].Input.Expression);
    FString ParameterName;
    if (Parameter) {
      ParameterName = Parameter->ParameterName.ToString();
    }

    if (ParameterName.IsEmpty()) {
      // In case, treat the node as invalid. Break any user-made connections to
      // this node and don't attempt to remap it.
      for (UMaterialExpression* UserNode : Classification.UserAddedNodes) {
        for (FExpressionInput* Input : UserNode->GetInputs()) {
          if (Input->Expression == GetFeatureIdNode &&
              Input->OutputIndex == 0) {
            Input->Expression = nullptr;
          }
        }
      }
      continue;
    }

    FString Key = GetFeatureIdNode->GetDescription() + ParameterName;
    TArray<FExpressionInput*> Connections;
    for (UMaterialExpression* UserNode : Classification.UserAddedNodes) {
      for (FExpressionInput* Input : UserNode->GetInputs()) {
        // Look for user-made connections to this node.
        if (Input->Expression == GetFeatureIdNode && Input->OutputIndex == 0) {
          Connections.Add(Input);
          Input->Expression = nullptr;
        }
      }
    }
    ConnectionRemap.Emplace(MoveTemp(Key), MoveTemp(Connections));
  }

  // Determine which user-added connections to remap when regenerating the
  // property value retrieval nodes.
  for (const UMaterialExpressionCustom* GetPropertyValueNode :
       Classification.GetPropertyValueNodes) {
    int32 OutputIndex = 0;
    for (const FExpressionOutput& PropertyOutput :
         GetPropertyValueNode->Outputs) {
      FString Key = GetPropertyValueNode->GetDescription() +
                    PropertyOutput.OutputName.ToString();

      // Look for user-made connections to this property.
      TArray<FExpressionInput*> Connections;
      for (UMaterialExpression* UserNode : Classification.UserAddedNodes) {
        for (FExpressionInput* Input : UserNode->GetInputs()) {
          if (Input->Expression == GetPropertyValueNode &&
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
  for (UMaterialExpression* AutoGeneratedNode :
       Classification.AutoGeneratedNodes) {
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
    Layer->FunctionExpressions.Remove(AutoGeneratedNode);
#else
    Layer->GetExpressionCollection().RemoveExpression(AutoGeneratedNode);
#endif
  }
}

static void RemapUserConnections(
    UMaterialFunctionMaterialLayer* Layer,
    TMap<FString, TArray<FExpressionInput*>>& ConnectionRemap,
    UMaterialFunction* GetFeatureIdsFromAttributeFunction,
    UMaterialFunction* GetFeatureIdsFromTextureFunction) {

  MaterialNodeClassification Classification;
  ClassifyNodes(
      Layer,
      Classification,
      GetFeatureIdsFromAttributeFunction,
      GetFeatureIdsFromTextureFunction);

  for (UMaterialExpressionMaterialFunctionCall* GetFeatureIdNode :
       Classification.GetFeatureIdNodes) {
    const auto Inputs = GetFeatureIdNode->FunctionInputs;
    const auto Parameter =
        Cast<UMaterialExpressionParameter>(Inputs[0].Input.Expression);
    FString ParameterName = Parameter->ParameterName.ToString();

    FString Key = GetFeatureIdNode->GetDescription() + ParameterName;
    TArray<FExpressionInput*>* pConnections = ConnectionRemap.Find(Key);
    if (pConnections) {
      for (FExpressionInput* pConnection : *pConnections) {
        pConnection->Expression = GetFeatureIdNode;
        pConnection->OutputIndex = 0;
      }
    }
  }

  for (UMaterialExpressionCustom* GetPropertyValueNode :
       Classification.GetPropertyValueNodes) {
    int32 OutputIndex = 0;
    for (const FExpressionOutput& PropertyOutput :
         GetPropertyValueNode->Outputs) {
      FString Key = GetPropertyValueNode->Description +
                    PropertyOutput.OutputName.ToString();

      TArray<FExpressionInput*>* pConnections = ConnectionRemap.Find(Key);
      if (pConnections) {
        for (FExpressionInput* pConnection : *pConnections) {
          pConnection->Expression = GetPropertyValueNode;
          pConnection->OutputIndex = OutputIndex;
        }
      }

      ++OutputIndex;
    }
  }
}

// Increment constant that is used to space out the autogenerated nodes.
static const int32 Incr = 200;

namespace {
UMaterialExpressionMaterialFunctionCall* GenerateNodesForFeatureIdTexture(
    const FCesiumFeatureIdSetDescription& Description,
    TArray<UMaterialExpression*>& AutoGeneratedNodes,
    UMaterialFunctionMaterialLayer* TargetMaterialLayer,
    UMaterialFunction* GetFeatureIdsFromTextureFunction,
    int32& NodeX,
    int32& NodeY) {
  UMaterialExpressionScalarParameter* TexCoordsIndex =
      NewObject<UMaterialExpressionScalarParameter>(TargetMaterialLayer);
  TexCoordsIndex->ParameterName =
      FName(Description.Name + MaterialTexCoordIndexSuffix);
  TexCoordsIndex->DefaultValue = 0.0f;
  TexCoordsIndex->MaterialExpressionEditorX = NodeX;
  TexCoordsIndex->MaterialExpressionEditorY = NodeY;
  AutoGeneratedNodes.Add(TexCoordsIndex);

  NodeY += Incr;

  UMaterialExpressionTextureObjectParameter* FeatureIdTexture =
      NewObject<UMaterialExpressionTextureObjectParameter>(TargetMaterialLayer);
  FeatureIdTexture->ParameterName =
      FName(Description.Name + MaterialTextureSuffix);
  FeatureIdTexture->MaterialExpressionEditorX = NodeX;
  FeatureIdTexture->MaterialExpressionEditorY = NodeY;
  AutoGeneratedNodes.Add(FeatureIdTexture);

  NodeY += Incr;

  UMaterialExpressionVectorParameter* Channels =
      NewObject<UMaterialExpressionVectorParameter>(TargetMaterialLayer);
  Channels->ParameterName = FName(Description.Name + MaterialChannelsSuffix);
  Channels->DefaultValue = FLinearColor(0, 0, 0, 0);
  Channels->MaterialExpressionEditorX = NodeX;
  Channels->MaterialExpressionEditorY = NodeY;
  AutoGeneratedNodes.Add(Channels);

  NodeY += Incr;

  UMaterialExpressionScalarParameter* NumChannels =
      NewObject<UMaterialExpressionScalarParameter>(TargetMaterialLayer);
  NumChannels->ParameterName =
      FName(Description.Name + MaterialNumChannelsSuffix);
  NumChannels->DefaultValue = 0.0f;
  NumChannels->MaterialExpressionEditorX = NodeX;
  NumChannels->MaterialExpressionEditorY = NodeY;
  AutoGeneratedNodes.Add(NumChannels);

  NodeY -= 1.75 * Incr;
  NodeX += 2 * Incr;

  UMaterialExpressionMaterialFunctionCall* GetFeatureIdsFromTexture =
      NewObject<UMaterialExpressionMaterialFunctionCall>(TargetMaterialLayer);
  GetFeatureIdsFromTexture->MaterialFunction = GetFeatureIdsFromTextureFunction;
  GetFeatureIdsFromTexture->MaterialExpressionEditorX = NodeX;
  GetFeatureIdsFromTexture->MaterialExpressionEditorY = NodeY;

  GetFeatureIdsFromTextureFunction->GetInputsAndOutputs(
      GetFeatureIdsFromTexture->FunctionInputs,
      GetFeatureIdsFromTexture->FunctionOutputs);
  GetFeatureIdsFromTexture->FunctionInputs[0].Input.Expression = TexCoordsIndex;
  GetFeatureIdsFromTexture->FunctionInputs[1].Input.Expression =
      FeatureIdTexture;
  GetFeatureIdsFromTexture->FunctionInputs[2].Input.Expression = Channels;
  GetFeatureIdsFromTexture->FunctionInputs[3].Input.Expression = NumChannels;
  AutoGeneratedNodes.Add(GetFeatureIdsFromTexture);

  return GetFeatureIdsFromTexture;
}

UMaterialExpressionMaterialFunctionCall* GenerateNodesForFeatureIdAttribute(
    const FCesiumFeatureIdSetDescription& Description,
    TArray<UMaterialExpression*>& AutoGeneratedNodes,
    UMaterialFunctionMaterialLayer* TargetMaterialLayer,
    UMaterialFunction* GetFeatureIdsFromAttributeFunction,
    int32& NodeX,
    int32& NodeY) {
  UMaterialExpressionScalarParameter* TextureCoordinateIndex =
      NewObject<UMaterialExpressionScalarParameter>(TargetMaterialLayer);
  TextureCoordinateIndex->ParameterName = FName(Description.Name);
  TextureCoordinateIndex->DefaultValue = 0.0f;
  TextureCoordinateIndex->MaterialExpressionEditorX = NodeX;
  TextureCoordinateIndex->MaterialExpressionEditorY = NodeY;
  AutoGeneratedNodes.Add(TextureCoordinateIndex);

  NodeX += 2 * Incr;

  UMaterialExpressionMaterialFunctionCall* GetFeatureIdsFromAttribute =
      NewObject<UMaterialExpressionMaterialFunctionCall>(TargetMaterialLayer);
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

  return GetFeatureIdsFromAttribute;
}

void GenerateNodesForPropertyTable(
    const FCesiumPropertyTableDescription& PropertyTable,
    TArray<UMaterialExpression*>& AutoGeneratedNodes,
    UMaterialFunctionMaterialLayer* TargetMaterialLayer,
    int32& NodeX,
    int32& NodeY,
    UMaterialExpressionMaterialFunctionCall* GetFeatureIdCall) {
  int32 SectionLeft = NodeX;
  int32 SectionTop = NodeY;

  NodeX += 1.5 * Incr;

  UMaterialExpressionCustom* GetPropertyValuesFunction =
      NewObject<UMaterialExpressionCustom>(TargetMaterialLayer);
  GetPropertyValuesFunction->Inputs.Reserve(PropertyTable.Properties.Num() + 2);
  GetPropertyValuesFunction->Outputs.Reset(PropertyTable.Properties.Num() + 1);
  GetPropertyValuesFunction->Outputs.Add(FExpressionOutput(TEXT("Return")));
  GetPropertyValuesFunction->bShowOutputNameOnPin = true;
  GetPropertyValuesFunction->Code = "";
  GetPropertyValuesFunction->Description =
      "Get Property Values From " + PropertyTable.Name;
  GetPropertyValuesFunction->MaterialExpressionEditorX = NodeX;
  GetPropertyValuesFunction->MaterialExpressionEditorY = NodeY;
  AutoGeneratedNodes.Add(GetPropertyValuesFunction);

  FCustomInput& FeatureIDInput = GetPropertyValuesFunction->Inputs[0];
  FeatureIDInput.InputName = FName("FeatureID");
  FeatureIDInput.Input.Expression = GetFeatureIdCall;

  if (PropertyTable.Properties.Num()) {
    const FCesiumPropertyTablePropertyDescription& property =
        PropertyTable.Properties[0];
    FString PropertyDataName =
        CesiumEncodedFeaturesMetadata::createHlslSafeName(property.Name) +
        MaterialPropertyDataSuffix;

    // Just get the dimensions of the first property. All the properties will
    // have the same pixel dimensions since it is based on the feature count.
    GetPropertyValuesFunction->Code += "uint _czm_width;\nuint _czm_height;\n";
    GetPropertyValuesFunction->Code +=
        PropertyDataName + ".GetDimensions(_czm_width, _czm_height);\n";
    GetPropertyValuesFunction->Code +=
        "uint _czm_pixelX = FeatureID % _czm_width;\n";
    GetPropertyValuesFunction->Code +=
        "uint _czm_pixelY = FeatureID / _czm_width;\n";
  }

  NodeX = SectionLeft;

  GetPropertyValuesFunction->AdditionalOutputs.Reserve(
      PropertyTable.Properties.Num());
  for (const FCesiumPropertyTablePropertyDescription& property :
       PropertyTable.Properties) {
    if (property.EncodingDetails.Conversion ==
        ECesiumEncodedMetadataConversion::None) {
      continue;
    }

    NodeY += Incr;

    FString propertyName = createHlslSafeName(property.Name);

    UMaterialExpressionTextureObjectParameter* PropertyData =
        NewObject<UMaterialExpressionTextureObjectParameter>(
            TargetMaterialLayer);
    PropertyData->ParameterName = FName(getMaterialNameForPropertyTableProperty(
        PropertyTable.Name,
        propertyName));

    PropertyData->MaterialExpressionEditorX = NodeX;
    PropertyData->MaterialExpressionEditorY = NodeY;
    AutoGeneratedNodes.Add(PropertyData);

    // Example: "roofColor_DATA"
    FString PropertyDataName =
        CesiumEncodedFeaturesMetadata::createHlslSafeName(property.Name) +
        MaterialPropertyDataSuffix;

    FCustomInput& PropertyInput =
        GetPropertyValuesFunction->Inputs.Emplace_GetRef();
    PropertyInput.InputName = FName(PropertyDataName);
    PropertyInput.Input.Expression = PropertyData;

    FCustomOutput& PropertyOutput =
        GetPropertyValuesFunction->AdditionalOutputs.Emplace_GetRef();
    PropertyOutput.OutputName = FName(propertyName);
    GetPropertyValuesFunction->Outputs.Add(
        FExpressionOutput(PropertyOutput.OutputName));

    FString swizzle = "";
    switch (property.EncodingDetails.Type) {
    case ECesiumEncodedMetadataType::Vec2:
      PropertyOutput.OutputType = ECustomMaterialOutputType::CMOT_Float2;
      swizzle = "rg";
      break;
    case ECesiumEncodedMetadataType::Vec3:
      PropertyOutput.OutputType = ECustomMaterialOutputType::CMOT_Float3;
      swizzle = "rgb";
      break;
    case ECesiumEncodedMetadataType::Vec4:
      PropertyOutput.OutputType = ECustomMaterialOutputType::CMOT_Float4;
      swizzle = "rgba";
      break;
    case ECesiumEncodedMetadataType::Scalar:
      PropertyOutput.OutputType = ECustomMaterialOutputType::CMOT_Float1;
      swizzle = "r";
      break;
    };

    FString asComponentString =
        property.EncodingDetails.ComponentType ==
                ECesiumEncodedMetadataComponentType::Float
            ? "asfloat"
            : "asuint";

    // Example:
    // "color = asfloat(color_DATA.Load(int3(_czm_pixelX, _czm_pixelY,
    // 0)).rgb);"
    GetPropertyValuesFunction->Code +=
        propertyName + " = " + asComponentString + "(" + PropertyDataName +
        ".Load(int3(_czm_pixelX, _czm_pixelY, 0))." + swizzle + ");\n";
  }

  // Obligatory return code.
  GetPropertyValuesFunction->OutputType =
      ECustomMaterialOutputType::CMOT_Float1;
  GetPropertyValuesFunction->Code += "return FeatureID;";
}

void GenerateNodesForPropertyTexture(
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
  //    FeatureTextureLookup->Inputs.Reset(2 *
  //    featureTexture.Properties.Num());
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
  //          FName("FTX_" + featureTexture.Name + "_" + property.Name +
  //          "_TX");
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
  //          FName("FTX_" + featureTexture.Name + "_" + property.Name +
  //          "_UV");
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
  //      SelectTexCoords->FunctionInputs[0].Input.Expression =
  //      TexCoordsIndex; AutoGeneratedNodes.Add(SelectTexCoords);
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
  //        PropertyOutput.OutputType =
  //        ECustomMaterialOutputType::CMOT_Float2; break;
  //      case ECesiumPropertyType::Vec3:
  //        PropertyOutput.OutputType =
  //        ECustomMaterialOutputType::CMOT_Float3; break;
  //      case ECesiumPropertyType::Vec4:
  //        PropertyOutput.OutputType =
  //        ECustomMaterialOutputType::CMOT_Float4; break;
  //      // case ECesiumPropertyType::Scalar:
  //      default:
  //        PropertyOutput.OutputType =
  //        ECustomMaterialOutputType::CMOT_Float1;
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
  //          ".Sample(" + propertyTextureName + "Sampler, " + propertyUvName
  //          +
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

void GenerateMaterialNodes(
    UCesiumFeaturesMetadataComponent* pComponent,
    TArray<UMaterialExpression*>& AutoGeneratedNodes,
    TArray<UMaterialExpression*>& OneTimeGeneratedNodes,
    UMaterialFunction* GetFeatureIdsFromAttributeFunction,
    UMaterialFunction* GetFeatureIdsFromTextureFunction) {
  int32 NodeX = 0;
  int32 NodeY = 0;

  TSet<FString> GeneratedPropertyTableNames;

  int32 FeatureIdSectionLeft = NodeX;
  int32 PropertyTableSectionLeft = FeatureIdSectionLeft + 3 * Incr;

  for (const FCesiumFeatureIdSetDescription& featureIdSet :
       pComponent->FeatureIdSets) {
    if (featureIdSet.Type == ECesiumFeatureIdSetType::None) {
      continue;
    }

    UMaterialExpressionMaterialFunctionCall* GetFeatureIdCall = nullptr;
    if (featureIdSet.Type == ECesiumFeatureIdSetType::Texture) {
      GetFeatureIdCall = GenerateNodesForFeatureIdTexture(
          featureIdSet,
          AutoGeneratedNodes,
          pComponent->TargetMaterialLayer,
          GetFeatureIdsFromTextureFunction,
          NodeX,
          NodeY);
    } else {
      // Handle implicit feature IDs the same as feature ID attributes
      GetFeatureIdCall = GenerateNodesForFeatureIdAttribute(
          featureIdSet,
          AutoGeneratedNodes,
          pComponent->TargetMaterialLayer,
          GetFeatureIdsFromAttributeFunction,
          NodeX,
          NodeY);
    }

    if (featureIdSet.PropertyTableName.IsEmpty()) {
      continue;
    }

    const FCesiumPropertyTableDescription* pPropertyTable =
        pComponent->PropertyTables.FindByPredicate(
            [&name = featureIdSet.PropertyTableName](
                const FCesiumPropertyTableDescription& existingPropertyTable) {
              return existingPropertyTable.Name == name;
            });

    if (pPropertyTable) {
      NodeX = PropertyTableSectionLeft;

      GenerateNodesForPropertyTable(
          *pPropertyTable,
          AutoGeneratedNodes,
          pComponent->TargetMaterialLayer,
          NodeX,
          NodeY,
          GetFeatureIdCall);

      GeneratedPropertyTableNames.Add(pPropertyTable->Name);
    }

    NodeX = FeatureIdSectionLeft;
    NodeY += 2 * Incr;
  }

  NodeX = PropertyTableSectionLeft;

  // Generate nodes for any property tables that aren't linked to a feature ID
  // set.
  for (const FCesiumPropertyTableDescription& propertyTable :
       pComponent->PropertyTables) {
    if (!GeneratedPropertyTableNames.Find(propertyTable.Name)) {
      GenerateNodesForPropertyTable(
          propertyTable,
          AutoGeneratedNodes,
          pComponent->TargetMaterialLayer,
          NodeX,
          NodeY,
          nullptr);
      NodeX = PropertyTableSectionLeft;
      NodeY += Incr;
    }
  }

  // GenerateNodesForPropertyTextures

  NodeX = FeatureIdSectionLeft;
  NodeY = -2 * Incr;

  UMaterialExpressionFunctionInput* InputMaterial = nullptr;
#if ENGINE_MINOR_VERSION == 0
  for (UMaterialExpression* ExistingNode :
       pComponent->TargetMaterialLayer->FunctionExpressions) {
#else
  for (const TObjectPtr<UMaterialExpression>& ExistingNode :
       pComponent->TargetMaterialLayer->GetExpressionCollection().Expressions) {
#endif
    UMaterialExpressionFunctionInput* ExistingInputMaterial =
        Cast<UMaterialExpressionFunctionInput>(ExistingNode);
    if (ExistingInputMaterial) {
      InputMaterial = ExistingInputMaterial;
      break;
    }
  }

  if (!InputMaterial) {
    InputMaterial = NewObject<UMaterialExpressionFunctionInput>(
        pComponent->TargetMaterialLayer);
    InputMaterial->InputType =
        EFunctionInputType::FunctionInput_MaterialAttributes;
    InputMaterial->bUsePreviewValueAsDefault = true;
    InputMaterial->MaterialExpressionEditorX = NodeX;
    InputMaterial->MaterialExpressionEditorY = NodeY;
    OneTimeGeneratedNodes.Add(InputMaterial);
  }

  NodeX += PropertyTableSectionLeft + 3 * Incr;

  UMaterialExpressionSetMaterialAttributes* SetMaterialAttributes = nullptr;
#if ENGINE_MINOR_VERSION == 0
  for (UMaterialExpression* ExistingNode :
       pComponent->TargetMaterialLayer->FunctionExpressions) {
#else
  for (const TObjectPtr<UMaterialExpression>& ExistingNode :
       pComponent->TargetMaterialLayer->GetExpressionCollection().Expressions) {
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
        pComponent->TargetMaterialLayer);
    OneTimeGeneratedNodes.Add(SetMaterialAttributes);
  }

  SetMaterialAttributes->Inputs[0].Expression = InputMaterial;
  SetMaterialAttributes->MaterialExpressionEditorX = NodeX;
  SetMaterialAttributes->MaterialExpressionEditorY = NodeY;

  NodeX += 2 * Incr;

  UMaterialExpressionFunctionOutput* OutputMaterial = nullptr;
#if ENGINE_MINOR_VERSION == 0
  for (UMaterialExpression* ExistingNode :
       pComponent->TargetMaterialLayer->FunctionExpressions) {
#else
  for (const TObjectPtr<UMaterialExpression>& ExistingNode :
       pComponent->TargetMaterialLayer->GetExpressionCollection().Expressions) {
#endif
    UMaterialExpressionFunctionOutput* ExistingOutputMaterial =
        Cast<UMaterialExpressionFunctionOutput>(ExistingNode);
    if (ExistingOutputMaterial) {
      OutputMaterial = ExistingOutputMaterial;
      break;
    }
  }

  if (!OutputMaterial) {
    OutputMaterial = NewObject<UMaterialExpressionFunctionOutput>(
        pComponent->TargetMaterialLayer);
    OneTimeGeneratedNodes.Add(OutputMaterial);
  }

  OutputMaterial->MaterialExpressionEditorX = NodeX;
  OutputMaterial->MaterialExpressionEditorY = NodeY;
  OutputMaterial->A = FMaterialAttributesInput();
  OutputMaterial->A.Expression = SetMaterialAttributes;
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

  // UMaterialFunction* SelectTexCoordsFunction = LoadMaterialFunction(
  //    "/CesiumForUnreal/Materials/MaterialFunctions/CesiumSelectTexCoords.CesiumSelectTexCoords");
  UMaterialFunction* GetFeatureIdsFromAttributeFunction = LoadMaterialFunction(
      "/CesiumForUnreal/Materials/MaterialFunctions/CesiumGetFeatureIdsFromAttribute.CesiumGetFeatureIdsFromAttribute");
  UMaterialFunction* GetFeatureIdsFromTextureFunction = LoadMaterialFunction(
      "/CesiumForUnreal/Materials/MaterialFunctions/CesiumGetFeatureIdsFromTexture.CesiumGetFeatureIdsFromTexture");

  if (!GetFeatureIdsFromAttributeFunction ||
      !GetFeatureIdsFromTextureFunction) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Can't find the material functions necessary to generate material. Aborting."));
    return;
  }

  if (this->TargetMaterialLayer &&
      this->TargetMaterialLayer->GetPackage()->IsDirty()) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Can't regenerate a material layer that has unsaved changes. Please save your changes and try again."));
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

    // Create an Unreal material layer
    UMaterialFunctionMaterialLayerFactory* MaterialFactory =
        NewObject<UMaterialFunctionMaterialLayerFactory>();
    this->TargetMaterialLayer =
        (UMaterialFunctionMaterialLayer*)MaterialFactory->FactoryCreateNew(
            UMaterialFunctionMaterialLayer::StaticClass(),
            Package,
            *MaterialName,
            RF_Public | RF_Standalone | RF_Transactional,
            NULL,
            GWarn);
    FAssetRegistryModule::AssetCreated(this->TargetMaterialLayer);
    Package->FullyLoad();
    Package->SetDirtyFlag(true);
  }

  this->TargetMaterialLayer->PreEditChange(NULL);

  TMap<FString, TArray<FExpressionInput*>> ConnectionRemap;
  ClearAutoGeneratedNodes(
      this->TargetMaterialLayer,
      ConnectionRemap,
      GetFeatureIdsFromAttributeFunction,
      GetFeatureIdsFromTextureFunction);

  TArray<UMaterialExpression*> AutoGeneratedNodes;
  TArray<UMaterialExpression*> OneTimeGeneratedNodes;

  GenerateMaterialNodes(
      this,
      AutoGeneratedNodes,
      OneTimeGeneratedNodes,
      GetFeatureIdsFromAttributeFunction,
      GetFeatureIdsFromTextureFunction);

  // Add the generated nodes to the material.

  for (UMaterialExpression* AutoGeneratedNode : AutoGeneratedNodes) {
    // Mark as auto-generated. If the material is regenerated, we will look
    // for this exact description to determine whether it was autogenerated.

    AutoGeneratedNode->Desc = "AUTOGENERATED DO NOT EDIT";
#if ENGINE_MINOR_VERSION == 0
    this->TargetMaterialLayer->FunctionExpressions.Add(AutoGeneratedNode);
#else
    this->TargetMaterialLayer->GetExpressionCollection().AddExpression(
        AutoGeneratedNode);
#endif
  }

  for (UMaterialExpression* OneTimeGeneratedNode : OneTimeGeneratedNodes) {
#if ENGINE_MINOR_VERSION == 0
    this->TargetMaterialLayer->FunctionExpressions.Add(OneTimeGeneratedNode);
#else
    this->TargetMaterialLayer->GetExpressionCollection().AddExpression(
        OneTimeGeneratedNode);
#endif
  }

  RemapUserConnections(
      this->TargetMaterialLayer,
      ConnectionRemap,
      GetFeatureIdsFromAttributeFunction,
      GetFeatureIdsFromTextureFunction);

  // Let the material update itself if necessary
  this->TargetMaterialLayer->PostEditChange();

  // Make sure that any static meshes, etc using this material will stop
  // using the FMaterialResource of the original material, and will use the
  // new FMaterialResource created when we make a new UMaterial in place
  FGlobalComponentReregisterContext RecreateComponents;

  // If this is a new material, open the content browser to the auto-generated
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

  // Open the updated material in editor.
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
