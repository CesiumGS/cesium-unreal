// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumFeaturesMetadataComponent.h"
#include "Cesium3DTileset.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumModelMetadata.h"
#include "CesiumRuntime.h"
#include "EncodedFeaturesMetadata.h"
#include "EncodedMetadataConversions.h"
#include "GenerateMaterialUtility.h"
#include "UnrealMetadataConversions.h"

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
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Materials/MaterialExpressionIf.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionPerInstanceCustomData.h"
#include "Materials/MaterialExpressionRound.h"
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

using namespace EncodedFeaturesMetadata;
using namespace GenerateMaterialUtility;

namespace {
void AutoFillPropertyTableDescriptions(
    TArray<FCesiumPropertyTableDescription>& Descriptions,
    const FCesiumModelMetadata& ModelMetadata) {
  const TArray<FCesiumPropertyTable>& propertyTables =
      UCesiumModelMetadataBlueprintLibrary::GetPropertyTables(ModelMetadata);

  for (const auto& propertyTable : propertyTables) {
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
      auto pExistingProperty = pDescription->Properties.FindByPredicate(
          [&propertyName = propertyIt.Key](
              const FCesiumPropertyTablePropertyDescription& existingProperty) {
            return existingProperty.Name == propertyName;
          });

      if (pExistingProperty) {
        // We have already accounted for this property, but we may need to check
        // for its offset / scale, since they can differ from the class
        // property's definition.
        ECesiumMetadataType type = pExistingProperty->PropertyDetails.Type;
        switch (type) {
        case ECesiumMetadataType::Scalar:
        case ECesiumMetadataType::Vec2:
        case ECesiumMetadataType::Vec3:
        case ECesiumMetadataType::Vec4:
        case ECesiumMetadataType::Mat2:
        case ECesiumMetadataType::Mat3:
        case ECesiumMetadataType::Mat4:
          break;
        default:
          continue;
        }

        FCesiumMetadataValue offset =
            UCesiumPropertyTablePropertyBlueprintLibrary::GetOffset(
                propertyIt.Value);
        pExistingProperty->PropertyDetails.bHasOffset |=
            !UCesiumMetadataValueBlueprintLibrary::IsEmpty(offset);

        FCesiumMetadataValue scale =
            UCesiumPropertyTablePropertyBlueprintLibrary::GetOffset(
                propertyIt.Value);
        pExistingProperty->PropertyDetails.bHasScale |=
            !UCesiumMetadataValueBlueprintLibrary::IsEmpty(scale);

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

      FCesiumMetadataValue offset =
          UCesiumPropertyTablePropertyBlueprintLibrary::GetOffset(
              propertyIt.Value);
      property.PropertyDetails.bHasOffset =
          !UCesiumMetadataValueBlueprintLibrary::IsEmpty(offset);

      FCesiumMetadataValue scale =
          UCesiumPropertyTablePropertyBlueprintLibrary::GetOffset(
              propertyIt.Value);
      property.PropertyDetails.bHasScale =
          !UCesiumMetadataValueBlueprintLibrary::IsEmpty(scale);

      FCesiumMetadataValue noData =
          UCesiumPropertyTablePropertyBlueprintLibrary::GetNoDataValue(
              propertyIt.Value);
      property.PropertyDetails.bHasNoDataValue =
          !UCesiumMetadataValueBlueprintLibrary::IsEmpty(noData);

      FCesiumMetadataValue defaultValue =
          UCesiumPropertyTablePropertyBlueprintLibrary::GetDefaultValue(
              propertyIt.Value);
      property.PropertyDetails.bHasDefaultValue =
          !UCesiumMetadataValueBlueprintLibrary::IsEmpty(defaultValue);

      property.EncodingDetails = CesiumMetadataPropertyDetailsToEncodingDetails(
          property.PropertyDetails);
    }
  }
}

void AutoFillPropertyTextureDescriptions(
    TArray<FCesiumPropertyTextureDescription>& Descriptions,
    const FCesiumModelMetadata& ModelMetadata) {
  const TArray<FCesiumPropertyTexture>& propertyTextures =
      UCesiumModelMetadataBlueprintLibrary::GetPropertyTextures(ModelMetadata);

  for (const auto& propertyTexture : propertyTextures) {
    FString propertyTextureName = getNameForPropertyTexture(propertyTexture);
    FCesiumPropertyTextureDescription* pDescription =
        Descriptions.FindByPredicate(
            [&propertyTextureName =
                 propertyTextureName](const FCesiumPropertyTextureDescription&
                                          existingPropertyTexture) {
              return existingPropertyTexture.Name == propertyTextureName;
            });

    if (!pDescription) {
      pDescription = &Descriptions.Emplace_GetRef();
      pDescription->Name = propertyTextureName;
    }

    const TMap<FString, FCesiumPropertyTextureProperty>& properties =
        UCesiumPropertyTextureBlueprintLibrary::GetProperties(propertyTexture);
    for (const auto& propertyIt : properties) {
      auto pExistingProperty = pDescription->Properties.FindByPredicate(
          [&propertyName =
               propertyIt.Key](const FCesiumPropertyTexturePropertyDescription&
                                   existingProperty) {
            return propertyName == existingProperty.Name;
          });

      if (pExistingProperty) {
        // We have already accounted for this property, but we may need to check
        // for its offset / scale, since they can differ from the class
        // property's definition.
        ECesiumMetadataType type = pExistingProperty->PropertyDetails.Type;
        switch (type) {
        case ECesiumMetadataType::Scalar:
        case ECesiumMetadataType::Vec2:
        case ECesiumMetadataType::Vec3:
        case ECesiumMetadataType::Vec4:
        case ECesiumMetadataType::Mat2:
        case ECesiumMetadataType::Mat3:
        case ECesiumMetadataType::Mat4:
          break;
        default:
          continue;
        }

        FCesiumMetadataValue offset =
            UCesiumPropertyTexturePropertyBlueprintLibrary::GetOffset(
                propertyIt.Value);
        pExistingProperty->PropertyDetails.bHasOffset |=
            !UCesiumMetadataValueBlueprintLibrary::IsEmpty(offset);

        FCesiumMetadataValue scale =
            UCesiumPropertyTexturePropertyBlueprintLibrary::GetOffset(
                propertyIt.Value);
        pExistingProperty->PropertyDetails.bHasScale |=
            !UCesiumMetadataValueBlueprintLibrary::IsEmpty(scale);

        continue;
      }

      FCesiumPropertyTexturePropertyDescription& property =
          pDescription->Properties.Emplace_GetRef();
      property.Name = propertyIt.Key;

      const FCesiumMetadataValueType ValueType =
          UCesiumPropertyTexturePropertyBlueprintLibrary::GetValueType(
              propertyIt.Value);
      property.PropertyDetails.SetValueType(ValueType);
      property.PropertyDetails.ArraySize =
          UCesiumPropertyTexturePropertyBlueprintLibrary::GetArraySize(
              propertyIt.Value);
      property.PropertyDetails.bIsNormalized =
          UCesiumPropertyTexturePropertyBlueprintLibrary::IsNormalized(
              propertyIt.Value);

      FCesiumMetadataValue offset =
          UCesiumPropertyTexturePropertyBlueprintLibrary::GetOffset(
              propertyIt.Value);
      property.PropertyDetails.bHasOffset =
          !UCesiumMetadataValueBlueprintLibrary::IsEmpty(offset);

      FCesiumMetadataValue scale =
          UCesiumPropertyTexturePropertyBlueprintLibrary::GetOffset(
              propertyIt.Value);
      property.PropertyDetails.bHasScale =
          !UCesiumMetadataValueBlueprintLibrary::IsEmpty(scale);

      FCesiumMetadataValue noData =
          UCesiumPropertyTexturePropertyBlueprintLibrary::GetNoDataValue(
              propertyIt.Value);
      property.PropertyDetails.bHasNoDataValue =
          !UCesiumMetadataValueBlueprintLibrary::IsEmpty(noData);

      FCesiumMetadataValue defaultValue =
          UCesiumPropertyTexturePropertyBlueprintLibrary::GetDefaultValue(
              propertyIt.Value);
      property.PropertyDetails.bHasDefaultValue =
          !UCesiumMetadataValueBlueprintLibrary::IsEmpty(defaultValue);

      auto maybeTextureTransform = propertyIt.Value.getTextureTransform();
      if (maybeTextureTransform) {
        property.bHasKhrTextureTransform =
            (maybeTextureTransform->status() ==
             CesiumGltf::KhrTextureTransformStatus::Valid);
      }
    }
  }
}

void AutoFillFeatureIdSetDescriptions(
    TArray<FCesiumFeatureIdSetDescription>& Descriptions,
    const FCesiumPrimitiveFeatures& Features,
    const FCesiumPrimitiveFeatures* InstanceFeatures,
    const TArray<FCesiumPropertyTable>& PropertyTables) {
  TArray<FCesiumFeatureIdSet> featureIDSets =
      UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(Features);
  if (InstanceFeatures) {
    featureIDSets.Append(
        UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
            *InstanceFeatures));
  }
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

    if (type == ECesiumFeatureIdSetType::Texture) {
      FCesiumFeatureIdTexture featureIdTexture =
          UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDTexture(
              featureIDSet);
      auto maybeTextureTransform =
          featureIdTexture.getFeatureIdTextureView().getTextureTransform();
      if (maybeTextureTransform) {
        pDescription->bHasKhrTextureTransform =
            (maybeTextureTransform->status() ==
             CesiumGltf::KhrTextureTransformStatus::Valid);
      }
    }
  }
}

void AutoFillPropertyTextureNames(
    TSet<FString>& Names,
    const FCesiumPrimitiveMetadata& PrimitiveMetadata,
    const TArray<FCesiumPropertyTexture>& PropertyTextures) {
  const TArray<int64> propertyTextureIndices =
      UCesiumPrimitiveMetadataBlueprintLibrary::GetPropertyTextureIndices(
          PrimitiveMetadata);

  for (const int64& propertyTextureIndex : propertyTextureIndices) {
    if (propertyTextureIndex < 0 ||
        propertyTextureIndex >= PropertyTextures.Num()) {
      continue;
    }

    const FCesiumPropertyTexture& propertyTexture =
        PropertyTextures[propertyTextureIndex];
    FString propertyTextureName = getNameForPropertyTexture(propertyTexture);
    Names.Emplace(propertyTextureName);
  }
}

} // namespace

void UCesiumFeaturesMetadataComponent::AutoFill() {
  const ACesium3DTileset* pOwner = this->GetOwner<ACesium3DTileset>();
  if (!pOwner) {
    return;
  }

  Super::PreEditChange(NULL);

  // This assumes that the property tables are the same across all models in the
  // tileset, and that they all have the same schema.
  for (const UActorComponent* pComponent : pOwner->GetComponents()) {
    const UCesiumGltfComponent* pGltf = Cast<UCesiumGltfComponent>(pComponent);
    if (!pGltf) {
      continue;
    }

    const FCesiumModelMetadata& modelMetadata = pGltf->Metadata;
    AutoFillPropertyTableDescriptions(
        this->Description.ModelMetadata.PropertyTables,
        modelMetadata);
    AutoFillPropertyTextureDescriptions(
        this->Description.ModelMetadata.PropertyTextures,
        modelMetadata);

    TArray<USceneComponent*> childComponents;
    pGltf->GetChildrenComponents(false, childComponents);

    for (const USceneComponent* pChildComponent : childComponents) {
      const auto* pCesiumPrimitive = Cast<ICesiumPrimitive>(pChildComponent);
      if (!pCesiumPrimitive) {
        continue;
      }
      const CesiumPrimitiveData& primData =
          pCesiumPrimitive->getPrimitiveData();
      const FCesiumPrimitiveFeatures& primitiveFeatures = primData.Features;
      const TArray<FCesiumPropertyTable>& propertyTables =
          UCesiumModelMetadataBlueprintLibrary::GetPropertyTables(
              modelMetadata);
      const FCesiumPrimitiveFeatures* pInstanceFeatures = nullptr;
      const auto* pInstancedComponent =
          Cast<UCesiumGltfInstancedComponent>(pChildComponent);
      if (pInstancedComponent) {
        pInstanceFeatures = pInstancedComponent->pInstanceFeatures.Get();
      }
      AutoFillFeatureIdSetDescriptions(
          this->Description.PrimitiveFeatures.FeatureIdSets,
          primitiveFeatures,
          pInstanceFeatures,
          propertyTables);

      const FCesiumPrimitiveMetadata& primitiveMetadata = primData.Metadata;
      const TArray<FCesiumPropertyTexture>& propertyTextures =
          UCesiumModelMetadataBlueprintLibrary::GetPropertyTextures(
              modelMetadata);
      AutoFillPropertyTextureNames(
          this->Description.PrimitiveMetadata.PropertyTextureNames,
          primitiveMetadata,
          propertyTextures);
    }
  }

  Super::PostEditChange();
}

static FORCEINLINE UMaterialFunction* LoadMaterialFunction(const FName& Path) {
  return LoadObjFromPath<UMaterialFunction>(Path);
}

static const FString GetPropertyValuesPrefix = "Get Property Values From ";
static const FString ApplyValueTransformsPrefix = "Apply Value Transforms To ";

namespace {
struct FeaturesMetadataClassification : public MaterialNodeClassification {
  TArray<UMaterialExpressionMaterialFunctionCall*> GetFeatureIdNodes;
  TArray<UMaterialExpressionCustom*> GetPropertyValueNodes;
  TArray<UMaterialExpressionCustom*> ApplyValueTransformNodes;
  TArray<UMaterialExpressionIf*> IfNodes;
};

struct MaterialFunctionLibrary {
  UMaterialFunction* SelectTexCoords = nullptr;
  UMaterialFunction* TransformTexCoords = nullptr;
  UMaterialFunction* GetFeatureIdsFromAttribute = nullptr;
  UMaterialFunction* GetFeatureIdsFromTexture = nullptr;
  UMaterialFunction* GetFeatureIdsFromInstance = nullptr;

  MaterialFunctionLibrary()
      : SelectTexCoords(LoadMaterialFunction(
            "/CesiumForUnreal/Materials/MaterialFunctions/CesiumSelectTexCoords.CesiumSelectTexCoords")),
        TransformTexCoords(LoadMaterialFunction(
            "/CesiumForUnreal/Materials/MaterialFunctions/MF_CesiumTransformTextureCoordinates.MF_CesiumTransformTextureCoordinates")),
        GetFeatureIdsFromAttribute(LoadMaterialFunction(
            "/CesiumForUnreal/Materials/MaterialFunctions/CesiumGetFeatureIdsFromAttribute.CesiumGetFeatureIdsFromAttribute")),
        GetFeatureIdsFromTexture(LoadMaterialFunction(
            "/CesiumForUnreal/Materials/MaterialFunctions/CesiumGetFeatureIdsFromTexture.CesiumGetFeatureIdsFromTexture")),
        GetFeatureIdsFromInstance(LoadMaterialFunction(
            "/CesiumForUnreal/Materials/MaterialFunctions/CesiumGetFeatureIdsFromInstance.CesiumGetFeatureIdsFromInstance")) {
  }

  bool isValid() const {
    return SelectTexCoords != nullptr && TransformTexCoords != nullptr &&
           GetFeatureIdsFromAttribute != nullptr &&
           GetFeatureIdsFromTexture != nullptr &&
           GetFeatureIdsFromInstance != nullptr;
  }
};
} // namespace

static FeaturesMetadataClassification ClassifyNodes(
    const UMaterialFunctionMaterialLayer* Layer,
    const MaterialFunctionLibrary& FunctionLibrary) {
  const UMaterialFunction* GetFeatureIdsFromAttributeFunction =
      FunctionLibrary.GetFeatureIdsFromAttribute;
  const UMaterialFunction* GetFeatureIdsFromTextureFunction =
      FunctionLibrary.GetFeatureIdsFromTexture;
  const UMaterialFunction* GetFeatureIdsFromInstanceFunction =
      FunctionLibrary.GetFeatureIdsFromInstance;

  FeaturesMetadataClassification Classification;

  for (const TObjectPtr<UMaterialExpression>& Node :
       Layer->GetExpressionCollection().Expressions) {
    // Check if this node is marked as autogenerated.
    if (Node->Desc.StartsWith(
            AutogeneratedMessage,
            ESearchCase::Type::CaseSensitive)) {
      Classification.AutoGeneratedNodes.Add(Node);

      UMaterialExpressionCustom* CustomNode =
          Cast<UMaterialExpressionCustom>(Node);
      if (CustomNode &&
          CustomNode->Description.Contains(GetPropertyValuesPrefix)) {
        Classification.GetPropertyValueNodes.Add(CustomNode);
        continue;
      }

      if (CustomNode &&
          CustomNode->Description.Contains(ApplyValueTransformsPrefix)) {
        Classification.ApplyValueTransformNodes.Add(CustomNode);
        continue;
      }

      // If nodes are added in when feature ID sets specify a null feature ID
      // value, when properties specify a "no data" value, and when properties
      // specify a default value.
      UMaterialExpressionIf* IfNode = Cast<UMaterialExpressionIf>(Node);
      if (IfNode) {
        Classification.IfNodes.Add(IfNode);
        continue;
      }

      UMaterialExpressionMaterialFunctionCall* FunctionCallNode =
          Cast<UMaterialExpressionMaterialFunctionCall>(Node);
      if (!FunctionCallNode)
        continue;

      const FName& name = FunctionCallNode->MaterialFunction->GetFName();
      if (name == GetFeatureIdsFromAttributeFunction->GetFName() ||
          name == GetFeatureIdsFromTextureFunction->GetFName() ||
          name == GetFeatureIdsFromInstanceFunction->GetFName()) {
        Classification.GetFeatureIdNodes.Add(FunctionCallNode);
      }
    } else {
      Classification.UserAddedNodes.Add(Node);
    }
  }

  return Classification;
}

static void ClearAutoGeneratedNodes(
    UMaterialFunctionMaterialLayer* Layer,
    TMap<FString, TMap<FString, const FExpressionInput*>>& ConnectionInputRemap,
    TMap<FString, TArray<FExpressionInput*>>& ConnectionOutputRemap,
    const MaterialFunctionLibrary& FunctionLibrary) {

  FeaturesMetadataClassification Classification =
      ClassifyNodes(Layer, FunctionLibrary);

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
        for (FExpressionInput* Input : UserNode->GetInputsView()) {
          if (Input->Expression == GetFeatureIdNode &&
              Input->OutputIndex == 0) {
            Input->Expression = nullptr;
          }
        }
      }
      continue;
    }

    // It's not as easy to distinguish the material function calls from each
    // other. Try using the name of the first valid input (the texture
    // coordinate index name), which should be different for each feature ID
    // set.
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
        for (FExpressionInput* Input : UserNode->GetInputsView()) {
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
      for (FExpressionInput* Input : UserNode->GetInputsView()) {
        // Look for user-made connections to this node.
        if (Input->Expression == GetFeatureIdNode && Input->OutputIndex == 0) {
          Connections.Add(Input);
          Input->Expression = nullptr;
        }
      }
    }
    ConnectionOutputRemap.Emplace(MoveTemp(Key), MoveTemp(Connections));
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
        for (FExpressionInput* Input : UserNode->GetInputsView()) {
          if (Input->Expression == GetPropertyValueNode &&
              Input->OutputIndex == OutputIndex) {
            Connections.Add(Input);
            Input->Expression = nullptr;
          }
        }
      }

      ConnectionOutputRemap.Emplace(MoveTemp(Key), MoveTemp(Connections));
      ++OutputIndex;
    }
  }

  // Determine which user-added connections to remap when regenerating the
  // value transform nodes.
  for (const UMaterialExpressionCustom* ApplyValueTransformNode :
       Classification.ApplyValueTransformNodes) {
    int32 OutputIndex = 0;
    for (const FExpressionOutput& PropertyOutput :
         ApplyValueTransformNode->Outputs) {
      FString Key = ApplyValueTransformNode->GetDescription() +
                    PropertyOutput.OutputName.ToString();

      // Look for user-made connections to this property.
      TArray<FExpressionInput*> Connections;
      for (UMaterialExpression* UserNode : Classification.UserAddedNodes) {
        for (FExpressionInput* Input : UserNode->GetInputsView()) {
          if (Input->Expression == ApplyValueTransformNode &&
              Input->OutputIndex == OutputIndex) {
            Connections.Add(Input);
            Input->Expression = nullptr;
          }
        }
      }

      ConnectionOutputRemap.Emplace(MoveTemp(Key), MoveTemp(Connections));
      ++OutputIndex;
    }
  }

  // Determine which user-added connections to remap when regenerating the if
  // statements for null feature IDs.
  for (const UMaterialExpressionIf* IfNode : Classification.IfNodes) {
    // Distinguish the if statements from each other using A and B. If both of
    // these nodes have been disconnected, then treat this node as invalid.
    FString IfNodeName;

    UMaterialExpressionParameter* Parameter =
        Cast<UMaterialExpressionParameter>(IfNode->A.Expression);
    if (Parameter) {
      IfNodeName += Parameter->GetParameterName().ToString();
    } else if (IfNode->A.Expression) {
      TArray<FExpressionOutput>& Outputs = IfNode->A.Expression->GetOutputs();
      if (Outputs.Num() > 0) {
        IfNodeName += Outputs[IfNode->A.OutputIndex].OutputName.ToString();
      }
    }

    Parameter = Cast<UMaterialExpressionParameter>(IfNode->B.Expression);
    if (Parameter) {
      IfNodeName += Parameter->GetParameterName().ToString();
    } else if (IfNode->B.Expression) {
      TArray<FExpressionOutput>& Outputs = IfNode->B.Expression->GetOutputs();
      if (Outputs.Num() > 0) {
        IfNodeName += Outputs[IfNode->B.OutputIndex].OutputName.ToString();
      }
    }

    if (IfNodeName.IsEmpty()) {
      // In case, treat the node as invalid. Break any user-made connections to
      // this node and don't attempt to remap it.
      for (UMaterialExpression* UserNode : Classification.UserAddedNodes) {
        for (FExpressionInput* Input : UserNode->GetInputsView()) {
          if (Input->Expression == IfNode && Input->OutputIndex == 0) {
            Input->Expression = nullptr;
          }
        }
      }
      continue;
    }

    FString Key = IfNode->GetDescription() + IfNodeName;
    TArray<FExpressionInput*> Connections;
    for (UMaterialExpression* UserNode : Classification.UserAddedNodes) {
      for (FExpressionInput* Input : UserNode->GetInputsView()) {
        // Look for user-made connections to this node.
        if (Input->Expression == IfNode && Input->OutputIndex == 0) {
          Connections.Add(Input);
          Input->Expression = nullptr;
        }
      }
    }
    ConnectionOutputRemap.Emplace(Key, MoveTemp(Connections));

    // Also save any user inputs to the if statement. Be sure to ignore
    // connections to autogenerated nodes.
    TMap<FString, const FExpressionInput*> InputConnections;
    if (IfNode->AGreaterThanB.Expression &&
        !IfNode->AGreaterThanB.Expression->Desc.StartsWith(
            AutogeneratedMessage,
            ESearchCase::Type::CaseSensitive)) {
      InputConnections.Emplace(TEXT("AGreaterThanB"), &IfNode->AGreaterThanB);
    }

    if (IfNode->ALessThanB.Expression &&
        !IfNode->ALessThanB.Expression->Desc.StartsWith(
            AutogeneratedMessage,
            ESearchCase::Type::CaseSensitive)) {
      InputConnections.Emplace(TEXT("ALessThanB"), &IfNode->ALessThanB);
    }

    if (IfNode->AEqualsB.Expression &&
        !IfNode->AEqualsB.Expression->Desc.StartsWith(
            AutogeneratedMessage,
            ESearchCase::Type::CaseSensitive)) {
      InputConnections.Emplace(TEXT("AEqualsB"), &IfNode->AEqualsB);
    }

    ConnectionInputRemap.Emplace(MoveTemp(Key), MoveTemp(InputConnections));
  }

  // Remove auto-generated nodes.
  for (UMaterialExpression* AutoGeneratedNode :
       Classification.AutoGeneratedNodes) {
    Layer->GetExpressionCollection().RemoveExpression(AutoGeneratedNode);
  }
}

static void RemapUserConnections(
    UMaterialFunctionMaterialLayer* Layer,
    TMap<FString, TMap<FString, const FExpressionInput*>>& ConnectionInputRemap,
    TMap<FString, TArray<FExpressionInput*>>& ConnectionOutputRemap,
    const MaterialFunctionLibrary& FunctionLibrary) {

  FeaturesMetadataClassification Classification =
      ClassifyNodes(Layer, FunctionLibrary);

  for (UMaterialExpressionMaterialFunctionCall* GetFeatureIdNode :
       Classification.GetFeatureIdNodes) {
    const auto Inputs = GetFeatureIdNode->FunctionInputs;
    if (!Inputs.IsEmpty()) {
      const auto Parameter =
          Cast<UMaterialExpressionParameter>(Inputs[0].Input.Expression);
      FString ParameterName = Parameter->ParameterName.ToString();

      FString Key = GetFeatureIdNode->GetDescription() + ParameterName;
      TArray<FExpressionInput*>* pConnections = ConnectionOutputRemap.Find(Key);
      if (pConnections) {
        for (FExpressionInput* pConnection : *pConnections) {
          pConnection->Connect(0, GetFeatureIdNode);
        }
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

      TArray<FExpressionInput*>* pConnections = ConnectionOutputRemap.Find(Key);
      if (pConnections) {
        for (FExpressionInput* pConnection : *pConnections) {
          pConnection->Connect(OutputIndex, GetPropertyValueNode);
        }
      }

      ++OutputIndex;
    }
  }

  for (UMaterialExpressionCustom* ApplyValueTransformNode :
       Classification.ApplyValueTransformNodes) {
    int32 OutputIndex = 0;
    for (const FExpressionOutput& PropertyOutput :
         ApplyValueTransformNode->Outputs) {
      FString Key = ApplyValueTransformNode->Description +
                    PropertyOutput.OutputName.ToString();

      TArray<FExpressionInput*>* pConnections = ConnectionOutputRemap.Find(Key);
      if (pConnections) {
        for (FExpressionInput* pConnection : *pConnections) {
          pConnection->Connect(OutputIndex, ApplyValueTransformNode);
        }
      }

      ++OutputIndex;
    }
  }

  for (UMaterialExpressionIf* IfNode : Classification.IfNodes) {
    FString IfNodeName;

    FString AName;
    UMaterialExpressionParameter* Parameter =
        Cast<UMaterialExpressionParameter>(IfNode->A.Expression);
    if (Parameter) {
      AName = Parameter->GetParameterName().ToString();
    } else if (IfNode->A.Expression) {
      TArray<FExpressionOutput>& Outputs = IfNode->A.Expression->GetOutputs();
      if (Outputs.Num() > 0) {
        AName = Outputs[IfNode->A.OutputIndex].OutputName.ToString();
      }
    }

    FString BName;
    Parameter = Cast<UMaterialExpressionParameter>(IfNode->B.Expression);
    if (Parameter) {
      BName = Parameter->GetParameterName().ToString();
    } else if (IfNode->B.Expression) {
      TArray<FExpressionOutput>& Outputs = IfNode->B.Expression->GetOutputs();
      if (Outputs.Num() > 0) {
        BName = Outputs[IfNode->B.OutputIndex].OutputName.ToString();
      }
    }
    IfNodeName = AName + BName;

    FString Key = IfNode->GetDescription() + IfNodeName;
    TArray<FExpressionInput*>* pConnections = ConnectionOutputRemap.Find(Key);
    if (pConnections) {
      for (FExpressionInput* pConnection : *pConnections) {
        pConnection->Connect(0, IfNode);
      }
    }

    if (AName.Contains(MaterialPropertyHasValueSuffix)) {
      // Skip the if statement that handles omitted properties. All connections
      // to this node are meant to be autogenerated.
      continue;
    }

    bool isNoDataIfStatement = BName.Contains("NoData");

    TMap<FString, const FExpressionInput*>* pInputConnections =
        ConnectionInputRemap.Find(Key);
    if (pInputConnections) {
      const FExpressionInput** ppAGreaterThanB =
          pInputConnections->Find(TEXT("AGreaterThanB"));
      if (ppAGreaterThanB && *ppAGreaterThanB) {
        IfNode->AGreaterThanB = **ppAGreaterThanB;
      }

      const FExpressionInput** ppALessThanB =
          pInputConnections->Find(TEXT("ALessThanB"));
      if (ppALessThanB && *ppALessThanB) {
        IfNode->ALessThanB = **ppALessThanB;
      }

      if (isNoDataIfStatement && IfNode->AEqualsB.Expression) {
        // If this node is comparing the "no data" value, the property may also
        // have a default value. If it does, it will have already been connected
        // to this expression; don't overwrite it.
        continue;
      }

      const FExpressionInput** ppAEqualsB =
          pInputConnections->Find(TEXT("AEqualsB"));
      if (ppAEqualsB && *ppAEqualsB) {
        IfNode->AEqualsB = **ppAEqualsB;
      }
    }
  }
}

namespace {
/**
 * @brief Generates code for assembling metadata values from a scalar property
 * texture property.
 */
FString GenerateCodeForScalarPropertyTextureProperty(
    const FString& PropertyName,
    const FString& PropertyChannelsName,
    const FCesiumMetadataPropertyDetails& PropertyDetails) {
  // Example: "heightResult"
  FString PropertyResultName = PropertyName + "Result";
  // clang-format off
  // Example: "uint heightResult = 0;"
  FString code = "uint " + PropertyResultName + " = 0;\n";
  // clang-format on
  FString SampleString = "sample = asuint(f.Get(sampleColor, channel));\n";

  uint32 byteSize = GetMetadataTypeByteSize(
      PropertyDetails.Type,
      PropertyDetails.ComponentType);
  if (byteSize == 1) {
    // clang-format off
    code += "channel = uint(f.Get(" + PropertyChannelsName + ", 0));\n" +
            SampleString +
            PropertyResultName + " = sample;\n";
    // clang-format on
  } else {
    // clang-format off
    FString byteSizeString = std::to_string(byteSize).c_str();
    code += "byteOffset = 0;\n"
            "for (uint i = 0; i < " + byteSizeString + "; i++) {\n"
            "  channel = uint(f.Get(" + PropertyChannelsName + ", i));\n" +
            "  " + SampleString +
            "  " + PropertyResultName + " = " +
            PropertyResultName + " | (sample << byteOffset);\n"
            "  byteOffset += 8;\n"
            "}\n";
    // clang-format on
  }

  FString OutputName = PropertyName;
  if (PropertyDetails.bIsNormalized || PropertyDetails.bHasOffset ||
      PropertyDetails.bHasScale) {
    OutputName += MaterialPropertyRawSuffix;
  }

  switch (PropertyDetails.ComponentType) {
  case ECesiumMetadataComponentType::Float32:
    code += OutputName + " = asfloat(" + PropertyResultName + ");\n";
    break;
  case ECesiumMetadataComponentType::Int8:
  case ECesiumMetadataComponentType::Int16:
  case ECesiumMetadataComponentType::Int32:
    code += OutputName + " = asint(" + PropertyResultName + ");\n";
  default:
    code += OutputName + " = " + PropertyResultName + ";\n";
    break;
  }

  return code;
}

/**
 * @brief Generates code for assembling metadata values from a vec2 property
 * texture property.
 */
FString GenerateCodeForVec2PropertyTextureProperty(
    const FString& PropertyName,
    const FString& PropertyChannelsName,
    const FCesiumMetadataPropertyDetails& PropertyDetails) {
  FString ComponentString;
  switch (PropertyDetails.ComponentType) {
  case ECesiumMetadataComponentType::Uint8:
  case ECesiumMetadataComponentType::Uint16:
    ComponentString = "uint";
    break;
  case ECesiumMetadataComponentType::Int8:
  case ECesiumMetadataComponentType::Int16:
    ComponentString = "int";
    break;
  default:
    // Only 1 or 2-byte components are supported.
    return FString();
  }

  // Example: "sample = asuint(f.Get(sampleColor, channel));"
  FString SampleString =
      "sample = as" + ComponentString + "(f.Get(sampleColor, channel));\n";
  // Example: "uint2"
  FString TypeString = ComponentString + "2";
  // Example: "dimensionsResult"
  FString PropertyResultName = PropertyName + "Result";
  // Example: "uint2 dimensionsResult = uint2(0,0);"
  FString code =
      TypeString + " " + PropertyResultName + " = " + TypeString + "(0, 0);\n";

  if (GetMetadataTypeByteSize(
          PropertyDetails.Type,
          PropertyDetails.ComponentType) == 1) {
    // clang-format off
    code += "channel = uint(f.Get(" + PropertyChannelsName + ", 0));\n" +
            SampleString +
            PropertyResultName + ".x = sample;\n"
            "channel = uint(f.Get(" + PropertyChannelsName + ", 1));\n" +
            SampleString +
            PropertyResultName + ".y = sample;\n";
    // clang-format on
  } else {
    // clang-format off
    code += "channel = uint(f.Get(" + PropertyChannelsName + ", 0));\n" +
            SampleString +
            PropertyResultName + ".x = sample;\n"
            "channel = uint(f.Get(" + PropertyChannelsName + ", 1));\n" +
            SampleString +
            PropertyResultName + ".x = " + PropertyResultName +
            ".x | (sample << 8);\n "
            "channel = uint(f.Get(" + PropertyChannelsName + ", 2));\n" +
            SampleString +
            PropertyResultName + ".y = sample;\n"
            "channel = uint(f.Get(" + PropertyChannelsName + ", 3));\n" +
            SampleString +
            PropertyResultName + ".y = " + PropertyResultName +
            ".y | (sample << 8);\n ";
    // clang-format on
  }

  return code;
}

/**
 * @brief Generates code for assembling metadata values from a vecN property
 * texture property, assuming it contains single-byte components.
 */
FString GenerateCodeForVecNPropertyTextureProperty(
    const FString& PropertyName,
    const FString& PropertyChannelsName,
    ECesiumMetadataComponentType ComponentType,
    uint32 Count) {
  FString ComponentString;
  // Only single-byte components are supported.
  switch (ComponentType) {
  case ECesiumMetadataComponentType::Uint8:
    ComponentString = "uint";
    break;
  case ECesiumMetadataComponentType::Int8:
    ComponentString = "int";
    break;
  default:
    return FString();
  }

  FString CountString;
  FString ZeroString;
  switch (Count) {
  case 2:
    CountString = "2";
    ZeroString = "(0, 0)";
    break;
  case 3:
    CountString = "3";
    ZeroString = "(0, 0, 0)";
    break;
  case 4:
    CountString = "4";
    ZeroString = "(0, 0, 0, 0)";
    break;
  default:
    return FString();
  }

  // Example: "uint4"
  FString TypeString = ComponentString + CountString;
  // Example: "colorResult"
  FString PropertyResultName = PropertyName + "Result";
  // Example: "sample = asuint(f.Get(sampleColor, channel));"
  FString SampleString =
      "sample = as" + ComponentString + "(f.Get(sampleColor, channel));\n ";

  // Example: "uint4 colorResult = uint4(0, 0, 0, 0);"
  // clang-format off
  FString code =
      TypeString + " " + PropertyResultName + " = " + TypeString + ZeroString +
      ";\n"
      "channel = uint(f.Get(" + PropertyChannelsName + ", 0));\n" +
      SampleString +
      PropertyResultName + ".x = sample;\n"
      "channel = uint(f.Get(" + PropertyChannelsName + ", 1));\n" +
      SampleString +
      PropertyResultName + ".y = sample;\n";
  // clang-format on

  if (Count >= 3) {
    // clang-format off
    code += "channel = uint(f.Get(" + PropertyChannelsName + ", 2));\n" +
            SampleString +
            PropertyResultName + ".z = sample;\n";
    // clang-format on
  }

  if (Count == 4) {
    // clang-format off
    code += "channel = uint(f.Get(" + PropertyChannelsName + ", 3));\n" +
            SampleString +
            PropertyResultName + ".w = sample;\n";
    // clang-format on
  }

  return code;
}

/**
 * @brief Generates code for assembling metadata values from a property texture
 * property, depending on its type.
 */
FString GenerateCodeForPropertyTextureProperty(
    const FString& PropertyName,
    const FString& PropertyUVName,
    const FString& PropertyDataName,
    const FString& PropertyChannelsName,
    const FCesiumMetadataPropertyDetails& PropertyDetails) {
  // Example: sampleColor = Height_DATA.Sample(Height_DATASampler, Height_UV);
  // clang-format off
  FString code =
    "sampleColor = " +
    PropertyDataName + ".Sample(" + PropertyDataName + "Sampler, " + PropertyUVName + ");\n";

  // clang-format on
  if (PropertyDetails.bIsArray) {
    if (GetMetadataTypeByteSize(
            PropertyDetails.Type,
            PropertyDetails.ComponentType) > 1) {
      // Only single-byte array values are supported.
      return FString();
    }

    return code + GenerateCodeForVecNPropertyTextureProperty(
                      PropertyName,
                      PropertyChannelsName,
                      PropertyDetails.ComponentType,
                      PropertyDetails.ArraySize);
  }

  switch (PropertyDetails.Type) {
  case ECesiumMetadataType::Scalar:
    return code + GenerateCodeForScalarPropertyTextureProperty(
                      PropertyName,
                      PropertyChannelsName,
                      PropertyDetails);
  case ECesiumMetadataType::Vec2:
    // Vec2s must be handled differently because they can consist of either
    // single-byte or double-byte components
    return code + GenerateCodeForVec2PropertyTextureProperty(
                      PropertyName,
                      PropertyChannelsName,
                      PropertyDetails);
  case ECesiumMetadataType::Vec3:
    return code + GenerateCodeForVecNPropertyTextureProperty(
                      PropertyName,
                      PropertyChannelsName,
                      PropertyDetails.ComponentType,
                      3);
  case ECesiumMetadataType::Vec4:
    return code + GenerateCodeForVecNPropertyTextureProperty(
                      PropertyName,
                      PropertyChannelsName,
                      PropertyDetails.ComponentType,
                      4);
  default:
    return FString();
  }
}

/**
 * @brief Generates the nodes necessary to sample feature IDs from a feature ID
 * texture.
 */
UMaterialExpressionMaterialFunctionCall* GenerateNodesForFeatureIdTexture(
    const FCesiumFeatureIdSetDescription& Description,
    TArray<UMaterialExpression*>& AutoGeneratedNodes,
    UMaterialFunctionMaterialLayer* TargetMaterialLayer,
    const MaterialFunctionLibrary& FunctionLibrary,
    int32& NodeX,
    int32& NodeY) {
  int32 MaximumParameterSectionX = 0;
  FString SafeName = createHlslSafeName(Description.Name);

  UMaterialExpressionScalarParameter* TexCoordsIndex =
      NewObject<UMaterialExpressionScalarParameter>(TargetMaterialLayer);
  TexCoordsIndex->ParameterName = FName(SafeName + MaterialTexCoordIndexSuffix);
  TexCoordsIndex->DefaultValue = 0.0f;
  TexCoordsIndex->MaterialExpressionEditorX = NodeX;
  TexCoordsIndex->MaterialExpressionEditorY = NodeY;
  AutoGeneratedNodes.Add(TexCoordsIndex);

  MaximumParameterSectionX = FMath::Max(
      MaximumParameterSectionX,
      Incr * GetNameLengthScalar(TexCoordsIndex->ParameterName));
  NodeY += 0.75 * Incr;

  UMaterialExpressionTextureObjectParameter* FeatureIdTexture =
      NewObject<UMaterialExpressionTextureObjectParameter>(TargetMaterialLayer);
  FeatureIdTexture->ParameterName = FName(SafeName + MaterialTextureSuffix);
  FeatureIdTexture->MaterialExpressionEditorX = NodeX;
  FeatureIdTexture->MaterialExpressionEditorY = NodeY;
  AutoGeneratedNodes.Add(FeatureIdTexture);

  MaximumParameterSectionX = FMath::Max(
      MaximumParameterSectionX,
      Incr * GetNameLengthScalar(FeatureIdTexture->ParameterName));
  NodeY += Incr;

  UMaterialExpressionScalarParameter* NumChannels =
      NewObject<UMaterialExpressionScalarParameter>(TargetMaterialLayer);
  NumChannels->ParameterName = FName(SafeName + MaterialNumChannelsSuffix);
  NumChannels->DefaultValue = 0.0f;
  NumChannels->MaterialExpressionEditorX = NodeX;
  NumChannels->MaterialExpressionEditorY = NodeY;
  AutoGeneratedNodes.Add(NumChannels);

  MaximumParameterSectionX = FMath::Max(
      MaximumParameterSectionX,
      Incr * GetNameLengthScalar(NumChannels->ParameterName));
  NodeY += 0.75 * Incr;

  UMaterialExpressionVectorParameter* Channels =
      NewObject<UMaterialExpressionVectorParameter>(TargetMaterialLayer);
  Channels->ParameterName = FName(SafeName + MaterialChannelsSuffix);
  Channels->DefaultValue = FLinearColor(0, 0, 0, 0);
  Channels->MaterialExpressionEditorX = NodeX;
  Channels->MaterialExpressionEditorY = NodeY;
  AutoGeneratedNodes.Add(Channels);

  // KHR_texture_transform parameters
  UMaterialExpressionVectorParameter* TransformScaleOffset = nullptr;
  UMaterialExpressionVectorParameter* TransformRotation = nullptr;

  if (Description.bHasKhrTextureTransform) {
    TransformScaleOffset =
        NewObject<UMaterialExpressionVectorParameter>(TargetMaterialLayer);
    TransformScaleOffset->ParameterName =
        FName(SafeName + MaterialTextureScaleOffsetSuffix);
    TransformScaleOffset->DefaultValue = {1, 1, 0, 0};
    TransformScaleOffset->MaterialExpressionEditorX = NodeX;
    TransformScaleOffset->MaterialExpressionEditorY = NodeY + 1.25 * Incr;
    AutoGeneratedNodes.Add(TransformScaleOffset);

    MaximumParameterSectionX = FMath::Max(
        MaximumParameterSectionX,
        Incr * GetNameLengthScalar(TransformScaleOffset->ParameterName));

    TransformRotation =
        NewObject<UMaterialExpressionVectorParameter>(TargetMaterialLayer);
    TransformRotation->ParameterName =
        FName(SafeName + MaterialTextureRotationSuffix);
    TransformRotation->DefaultValue = {0, 1, 0, 1};
    TransformRotation->MaterialExpressionEditorX = NodeX;
    TransformRotation->MaterialExpressionEditorY = NodeY + 2.5 * Incr;
    AutoGeneratedNodes.Add(TransformRotation);
  }

  NodeX += MaximumParameterSectionX + Incr;

  UMaterialExpressionAppendVector* AppendChannels =
      NewObject<UMaterialExpressionAppendVector>(TargetMaterialLayer);
  AppendChannels->MaterialExpressionEditorX = NodeX;
  AppendChannels->MaterialExpressionEditorY = NodeY;
  AppendChannels->A.Connect(0, Channels);
  AppendChannels->B.Connect(4, Channels);

  AutoGeneratedNodes.Add(AppendChannels);

  UMaterialExpressionAppendVector* AppendScaleOffset = nullptr;

  if (Description.bHasKhrTextureTransform) {
    AppendScaleOffset =
        NewObject<UMaterialExpressionAppendVector>(TargetMaterialLayer);
    AppendScaleOffset->MaterialExpressionEditorX = NodeX;
    AppendScaleOffset->MaterialExpressionEditorY =
        TransformScaleOffset->MaterialExpressionEditorY;
    AppendScaleOffset->A.Connect(0, TransformScaleOffset);
    AppendScaleOffset->B.Connect(4, TransformScaleOffset);
    AutoGeneratedNodes.Add(AppendScaleOffset);
  }

  NodeY -= 1.75 * Incr;
  NodeX += 1.25 * Incr;

  UMaterialExpressionMaterialFunctionCall* GetFeatureIdsFromTexture =
      NewObject<UMaterialExpressionMaterialFunctionCall>(TargetMaterialLayer);
  GetFeatureIdsFromTexture->MaterialFunction =
      FunctionLibrary.GetFeatureIdsFromTexture;
  GetFeatureIdsFromTexture->MaterialExpressionEditorX = NodeX;
  GetFeatureIdsFromTexture->MaterialExpressionEditorY = NodeY;

  FunctionLibrary.GetFeatureIdsFromTexture->GetInputsAndOutputs(
      GetFeatureIdsFromTexture->FunctionInputs,
      GetFeatureIdsFromTexture->FunctionOutputs);

  GetFeatureIdsFromTexture->FunctionInputs[0].Input.Expression = TexCoordsIndex;
  GetFeatureIdsFromTexture->FunctionInputs[1].Input.Expression =
      FeatureIdTexture;
  GetFeatureIdsFromTexture->FunctionInputs[2].Input.Expression = NumChannels;
  GetFeatureIdsFromTexture->FunctionInputs[3].Input.Expression = AppendChannels;

  if (Description.bHasKhrTextureTransform) {
    GetFeatureIdsFromTexture->FunctionInputs[4].Input.Connect(
        0,
        AppendScaleOffset);
    GetFeatureIdsFromTexture->FunctionInputs[5].Input.Connect(
        0,
        TransformRotation);
  }

  AutoGeneratedNodes.Add(GetFeatureIdsFromTexture);

  NodeX += 2 * Incr;

  return GetFeatureIdsFromTexture;
}

/**
 * @brief Generates the nodes necessary to sample feature IDs from a feature ID
 * attribute.
 */
UMaterialExpressionMaterialFunctionCall* GenerateNodesForFeatureIdAttribute(
    const FCesiumFeatureIdSetDescription& Description,
    TArray<UMaterialExpression*>& AutoGeneratedNodes,
    UMaterialFunctionMaterialLayer* TargetMaterialLayer,
    UMaterialFunction* GetFeatureIdsFromAttributeFunction,
    int32& NodeX,
    int32& NodeY) {
  FString SafeName = createHlslSafeName(Description.Name);
  UMaterialExpressionScalarParameter* TextureCoordinateIndex =
      NewObject<UMaterialExpressionScalarParameter>(TargetMaterialLayer);
  TextureCoordinateIndex->ParameterName = FName(SafeName);
  TextureCoordinateIndex->DefaultValue = 0.0f;
  TextureCoordinateIndex->MaterialExpressionEditorX = NodeX;
  TextureCoordinateIndex->MaterialExpressionEditorY = NodeY;
  AutoGeneratedNodes.Add(TextureCoordinateIndex);

  NodeX += Incr *
           (0.2f + GetNameLengthScalar(TextureCoordinateIndex->ParameterName));

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

  NodeX += 2 * Incr;

  return GetFeatureIdsFromAttribute;
}

/**
 * @brief Generates the nodes necessary to account for the null feature ID value
 * from a feature ID set.
 */
void GenerateNodesForNullFeatureId(
    const FCesiumFeatureIdSetDescription& Description,
    TArray<UMaterialExpression*>& AutoGeneratedNodes,
    UMaterialFunctionMaterialLayer* TargetMaterialLayer,
    int32& NodeX,
    int32& NodeY,
    UMaterialExpression* LastNode) {
  int32 SectionTop = NodeY;
  NodeY += 0.5 * Incr;

  FString SafeName = createHlslSafeName(Description.Name);
  UMaterialExpressionScalarParameter* NullFeatureId =
      NewObject<UMaterialExpressionScalarParameter>(TargetMaterialLayer);
  NullFeatureId->ParameterName = FName(SafeName + MaterialNullFeatureIdSuffix);
  NullFeatureId->DefaultValue = -1.0f;
  NullFeatureId->MaterialExpressionEditorX = NodeX;
  NullFeatureId->MaterialExpressionEditorY = NodeY;
  AutoGeneratedNodes.Add(NullFeatureId);

  NodeY = SectionTop;
  NodeX += Incr * (0.75 + GetNameLengthScalar(NullFeatureId->ParameterName));

  UMaterialExpressionIf* IfStatement =
      NewObject<UMaterialExpressionIf>(TargetMaterialLayer);

  IfStatement->A.Expression = LastNode;
  IfStatement->B.Expression = NullFeatureId;

  IfStatement->MaterialExpressionEditorX = NodeX;
  IfStatement->MaterialExpressionEditorY = NodeY;

  AutoGeneratedNodes.Add(IfStatement);
}

/**
 * @brief Generates the nodes necessary to apply property transforms to a
 * metadata property.
 */
void GenerateNodesForMetadataPropertyTransforms(
    const FCesiumMetadataPropertyDetails& PropertyDetails,
    ECesiumEncodedMetadataType Type,
    const FString& PropertyName,
    const FString& FullPropertyName,
    TArray<UMaterialExpression*>& AutoGeneratedNodes,
    UMaterialFunctionMaterialLayer* TargetMaterialLayer,
    int32& NodeX,
    int32& NodeY,
    UMaterialExpressionCustom* GetPropertyValuesFunction,
    int32 GetPropertyValuesOutputIndex) {
  int32 BeginSectionX = NodeX;
  int32 BeginSectionY = NodeY;

  UMaterialExpressionCustom* ApplyTransformsFunction = nullptr;
  UMaterialExpression* GetNoDataValueNode = nullptr;
  UMaterialExpression* GetDefaultValueNode = nullptr;
  UMaterialExpressionIf* NoDataIfNode = nullptr;

  // This section corresponds to the parameter nodes on the left that actually
  // supply the transform values for a property.
  int32 MaximumParameterSectionX = 0;

  TArray<UMaterialExpression*> NodesToMove;
  ECustomMaterialOutputType OutputType = GetOutputTypeForEncodedType(Type);

  if (PropertyDetails.bIsNormalized || PropertyDetails.bHasScale ||
      PropertyDetails.bHasOffset) {
    ApplyTransformsFunction =
        NewObject<UMaterialExpressionCustom>(TargetMaterialLayer);
    ApplyTransformsFunction->Code = "";
    ApplyTransformsFunction->Description =
        ApplyValueTransformsPrefix + PropertyName;
    ApplyTransformsFunction->MaterialExpressionEditorX =
        BeginSectionX + 0.5 * Incr;
    ApplyTransformsFunction->MaterialExpressionEditorY = NodeY;

    ApplyTransformsFunction->Inputs.Reserve(3);
    ApplyTransformsFunction->Outputs.Reset(2);
    ApplyTransformsFunction->AdditionalOutputs.Reserve(1);
    ApplyTransformsFunction->Outputs.Add(FExpressionOutput(FName("Raw Value")));
    ApplyTransformsFunction->bShowOutputNameOnPin = true;
    AutoGeneratedNodes.Add(ApplyTransformsFunction);
    NodesToMove.Add(ApplyTransformsFunction);

    FCustomInput& RawValueInput = ApplyTransformsFunction->Inputs[0];
    RawValueInput.InputName = FName("RawValue");
    RawValueInput.Input.Expression = GetPropertyValuesFunction;
    RawValueInput.Input.OutputIndex = GetPropertyValuesOutputIndex;

    FCustomOutput& TransformedOutput =
        ApplyTransformsFunction->AdditionalOutputs.Emplace_GetRef();
    TransformedOutput.OutputName = FName("TransformedValue");
    ApplyTransformsFunction->Outputs.Add(
        FExpressionOutput(TransformedOutput.OutputName));

    TransformedOutput.OutputType = OutputType;

    FString TransformCode = "TransformedValue = ";

    if (PropertyDetails.bIsNormalized) {
      // Normalization can be hardcoded because only normalized uint8s are
      // supported.
      TransformCode += "(RawValue / 255.0f)";
    } else {
      TransformCode += "RawValue";
    }

    if (PropertyDetails.bHasScale) {
      NodeY += Incr;
      UMaterialExpressionParameter* Parameter = GenerateParameterNode(
          TargetMaterialLayer,
          Type,
          FullPropertyName + MaterialPropertyScaleSuffix,
          BeginSectionX,
          NodeY);
      AutoGeneratedNodes.Add(Parameter);

      FString ScaleName = "Scale";

      FCustomInput& ScaleInput =
          ApplyTransformsFunction->Inputs.Emplace_GetRef();
      ScaleInput.InputName = FName(ScaleName);
      ScaleInput.Input.Expression = Parameter;

      TransformCode += " * " + ScaleName;

      MaximumParameterSectionX = FMath::Max(
          MaximumParameterSectionX,
          Incr * GetNameLengthScalar(Parameter->ParameterName));
    }

    if (PropertyDetails.bHasOffset) {
      NodeY += Incr;
      UMaterialExpressionParameter* Parameter = GenerateParameterNode(
          TargetMaterialLayer,
          Type,
          FullPropertyName + MaterialPropertyOffsetSuffix,
          BeginSectionX,
          NodeY);
      AutoGeneratedNodes.Add(Parameter);

      FString OffsetName = "Offset";

      FCustomInput& OffsetInput =
          ApplyTransformsFunction->Inputs.Emplace_GetRef();
      OffsetInput.InputName = FName(OffsetName);
      OffsetInput.Input.Expression = Parameter;

      TransformCode += " + " + OffsetName;

      MaximumParameterSectionX = FMath::Max(
          MaximumParameterSectionX,
          Incr * GetNameLengthScalar(Parameter->ParameterName));
    }

    // Example: TransformedValue = (RawValue / 255.0f) * Scale_VALUE +
    // Offset_VALUE;
    ApplyTransformsFunction->Code += TransformCode + ";\n";

    // Return the raw value.
    ApplyTransformsFunction->OutputType = OutputType;
    ApplyTransformsFunction->Code += "return RawValue;";

    NodeX +=
        Incr * (1 + GetNameLengthScalar(ApplyTransformsFunction->Description));
  }

  FString swizzle = GetSwizzleForEncodedType(Type);

  if (PropertyDetails.bHasNoDataValue) {
    NodeY += Incr;
    UMaterialExpressionParameter* Parameter = GenerateParameterNode(
        TargetMaterialLayer,
        Type,
        FullPropertyName + MaterialPropertyNoDataSuffix,
        BeginSectionX,
        NodeY);
    AutoGeneratedNodes.Add(Parameter);

    int32 NameLength = Incr * GetNameLengthScalar(Parameter->ParameterName);

    if (Type == ECesiumEncodedMetadataType::Scalar) {
      // No additional work needs to be done to retrieve the scalar, so don't
      // an extra unnecessary node.
      GetNoDataValueNode = Parameter;
    } else {
      // This is equivalent to a "MakeFloatN" function.
      UMaterialExpressionCustom* CustomFunction =
          NewObject<UMaterialExpressionCustom>(TargetMaterialLayer);
      CustomFunction->Description = "Get No Data Value For " + PropertyName;
      CustomFunction->MaterialExpressionEditorX = BeginSectionX + 0.5 * Incr;
      CustomFunction->MaterialExpressionEditorY = NodeY;

      CustomFunction->Outputs.Reset(1);
      CustomFunction->bShowOutputNameOnPin = true;
      NodesToMove.Add(CustomFunction);
      AutoGeneratedNodes.Add(CustomFunction);

      FString NoDataName = "NoData";
      FString InputName = NoDataName + MaterialPropertyValueSuffix;

      FCustomInput& NoDataInput = CustomFunction->Inputs[0];
      NoDataInput.InputName = FName(InputName);
      NoDataInput.Input.Expression = Parameter;

      CustomFunction->Outputs.Add(FExpressionOutput(FName(NoDataName)));
      CustomFunction->OutputType = OutputType;

      CustomFunction->Code = "return " + InputName + swizzle + ";\n";
      GetNoDataValueNode = CustomFunction;

      NameLength += GetNameLengthScalar(CustomFunction->Description) * Incr;
    }

    MaximumParameterSectionX =
        FMath::Max(MaximumParameterSectionX, 0.25 * Incr);
  }

  if (PropertyDetails.bHasDefaultValue) {
    NodeY += 0.75 * Incr;
    UMaterialExpressionParameter* Parameter = GenerateParameterNode(
        TargetMaterialLayer,
        Type,
        FullPropertyName + MaterialPropertyDefaultValueSuffix,
        BeginSectionX,
        NodeY);
    AutoGeneratedNodes.Add(Parameter);

    int32 NameLength = Incr * GetNameLengthScalar(Parameter->ParameterName);

    if (Type == ECesiumEncodedMetadataType::Scalar) {
      // No additional work needs to be done to retrieve the scalar, so don't
      // an extra unnecessary node.
      GetDefaultValueNode = Parameter;
    } else {
      // This is equivalent to a "MakeFloatN" function.
      UMaterialExpressionCustom* CustomFunction =
          NewObject<UMaterialExpressionCustom>(TargetMaterialLayer);
      CustomFunction->Description = "Get Default Value For " + PropertyName;
      CustomFunction->MaterialExpressionEditorX = BeginSectionX + 0.5 * Incr;
      CustomFunction->MaterialExpressionEditorY = NodeY;

      CustomFunction->Outputs.Reset(1);
      CustomFunction->bShowOutputNameOnPin = true;
      NodesToMove.Add(CustomFunction);
      AutoGeneratedNodes.Add(CustomFunction);

      FString DefaultName = "Default";
      FString InputName = DefaultName + MaterialPropertyValueSuffix;

      FCustomInput& DefaultInput = CustomFunction->Inputs[0];
      DefaultInput.InputName = FName(InputName);
      DefaultInput.Input.Expression = Parameter;

      CustomFunction->Outputs.Add(FExpressionOutput(FName("Default Value")));
      CustomFunction->OutputType = OutputType;

      // Example: Default = Default_VALUE.xyz;
      CustomFunction->Code = "return " + InputName + swizzle + ";\n";

      GetDefaultValueNode = CustomFunction;
      NameLength += GetNameLengthScalar(CustomFunction->Description) * Incr;
    }

    MaximumParameterSectionX = FMath::Max(
        MaximumParameterSectionX,
        Incr * GetNameLengthScalar(Parameter->ParameterName));
  }

  for (UMaterialExpression* Node : NodesToMove) {
    Node->MaterialExpressionEditorX += MaximumParameterSectionX;
  }
  NodesToMove.Empty();

  NodeX += FMath::Max(2 * Incr, MaximumParameterSectionX + Incr);

  // We want to return to the top of the section and work down again, without
  // overwriting NodeY. At the end, we use the maximum value to determine the
  // vertical extent of the entire section.
  int32 SectionNodeY = BeginSectionY;

  // Add if statement for resolving the no data / default values
  if (GetNoDataValueNode) {
    NodeX += Incr;

    NoDataIfNode = NewObject<UMaterialExpressionIf>(TargetMaterialLayer);
    NoDataIfNode->MaterialExpressionEditorX = NodeX;
    NoDataIfNode->MaterialExpressionEditorY = SectionNodeY;

    NoDataIfNode->B.Expression = GetNoDataValueNode;
    NoDataIfNode->AEqualsB.Expression = GetDefaultValueNode;

    if (ApplyTransformsFunction) {
      NoDataIfNode->A.Expression = ApplyTransformsFunction;
      NoDataIfNode->A.OutputIndex = 0;

      NoDataIfNode->AGreaterThanB.Expression = ApplyTransformsFunction;
      NoDataIfNode->AGreaterThanB.OutputIndex = 1;

      NoDataIfNode->ALessThanB.Expression = ApplyTransformsFunction;
      NoDataIfNode->ALessThanB.OutputIndex = 1;
    } else {
      NoDataIfNode->A.Expression = GetPropertyValuesFunction;
      NoDataIfNode->A.OutputIndex = GetPropertyValuesOutputIndex;

      NoDataIfNode->AGreaterThanB.Expression = GetPropertyValuesFunction;
      NoDataIfNode->AGreaterThanB.OutputIndex = GetPropertyValuesOutputIndex;

      NoDataIfNode->ALessThanB.Expression = GetPropertyValuesFunction;
      NoDataIfNode->ALessThanB.OutputIndex = GetPropertyValuesOutputIndex;
    }

    AutoGeneratedNodes.Add(NoDataIfNode);
    NodeX += 2 * Incr;
  }

  // If the property has a default value defined, it may be omitted from an
  // instance of a property table, texture, or attribute. In this case, the
  // default value should be used without needing to execute the
  // GetPropertyValues function. We check this with a scalar parameter that
  // acts as a boolean.
  if (GetDefaultValueNode) {
    UMaterialExpressionScalarParameter* HasValueParameter =
        NewObject<UMaterialExpressionScalarParameter>(TargetMaterialLayer);
    HasValueParameter->DefaultValue = 0.0f;
    HasValueParameter->ParameterName =
        FName(FullPropertyName + MaterialPropertyHasValueSuffix);
    HasValueParameter->MaterialExpressionEditorX = NodeX;
    HasValueParameter->MaterialExpressionEditorY = SectionNodeY;
    AutoGeneratedNodes.Add(HasValueParameter);

    NodeX += Incr * (1 + GetNameLengthScalar(HasValueParameter->ParameterName));
    UMaterialExpressionIf* IfStatement =
        NewObject<UMaterialExpressionIf>(TargetMaterialLayer);
    IfStatement->MaterialExpressionEditorX = NodeX;
    IfStatement->MaterialExpressionEditorY = SectionNodeY;

    IfStatement->A.Expression = HasValueParameter;
    IfStatement->ConstB = 1.0f;

    IfStatement->ALessThanB.Expression = GetDefaultValueNode;

    if (NoDataIfNode) {
      IfStatement->AGreaterThanB.Expression = NoDataIfNode;
      IfStatement->AEqualsB.Expression = NoDataIfNode;
    } else if (ApplyTransformsFunction) {
      IfStatement->AGreaterThanB.Expression = ApplyTransformsFunction;
      IfStatement->AGreaterThanB.OutputIndex = 1;

      IfStatement->AEqualsB.Expression = ApplyTransformsFunction;
      IfStatement->AEqualsB.OutputIndex = 1;
    } else {
      IfStatement->AGreaterThanB.Expression = GetPropertyValuesFunction;
      IfStatement->AGreaterThanB.OutputIndex = GetPropertyValuesOutputIndex;

      IfStatement->AEqualsB.Expression = GetPropertyValuesFunction;
      IfStatement->AEqualsB.OutputIndex = GetPropertyValuesOutputIndex;
    }

    AutoGeneratedNodes.Add(IfStatement);
  }

  if (SectionNodeY > NodeY) {
    NodeY = SectionNodeY;
  }

  NodeY += Incr;
}

/**
 * @brief Generates the nodes necessary to retrieve values from a property
 * table.
 */
void GenerateNodesForPropertyTable(
    const FCesiumPropertyTableDescription& PropertyTable,
    TArray<UMaterialExpression*>& AutoGeneratedNodes,
    UMaterialFunctionMaterialLayer* TargetMaterialLayer,
    int32& NodeX,
    int32& NodeY,
    UMaterialExpression* GetFeatureExpression) {
  int32 BeginSectionX = NodeX;
  // This value is used by parameters on the left side of the
  // "GetPropertyValues" function...
  int32 PropertyDataSectionY = NodeY - 0.5 * Incr;
  // ...whereas this value is used for parameters on the right side of the
  // function.
  int32 PropertyTransformsSectionY = NodeY + 20;

  UMaterialExpressionCustom* GetPropertyValuesFunction =
      NewObject<UMaterialExpressionCustom>(TargetMaterialLayer);
  GetPropertyValuesFunction->Inputs.Reserve(PropertyTable.Properties.Num() + 2);
  GetPropertyValuesFunction->Outputs.Reset(PropertyTable.Properties.Num() + 1);
  GetPropertyValuesFunction->Outputs.Add(FExpressionOutput(TEXT("Feature ID")));
  GetPropertyValuesFunction->bShowOutputNameOnPin = true;
  GetPropertyValuesFunction->Code = "";
  GetPropertyValuesFunction->Description =
      GetPropertyValuesPrefix + PropertyTable.Name;
  GetPropertyValuesFunction->MaterialExpressionEditorX = NodeX;
  GetPropertyValuesFunction->MaterialExpressionEditorY = NodeY;
  AutoGeneratedNodes.Add(GetPropertyValuesFunction);

  int32 GetPropertyValuesFunctionWidth =
      Incr * GetNameLengthScalar(GetPropertyValuesFunction->Description);

  // To prevent nodes from overlapping -- especially if they have really long
  // names -- the GetPropertyValuesFunction node will be shifted to the right
  // depending on the longest name among the parameters on the left.
  int32 MaximumPropertyDataSectionX = 0;
  // In a similar vein, this tracks the overall width of the property transforms
  // section. This will be added to NodeX at the end so that nodes can continue
  // to spawn horizontally.
  int32 MaximumPropertyTransformsSectionX = 0;

  // The nodes to the right of GetPropertyValuesFunction will also need to be
  // shifted, hence this array to keep track of them.
  TArray<UMaterialExpression*> PropertyTransformNodes;

  FCustomInput& FeatureIDInput = GetPropertyValuesFunction->Inputs[0];
  FeatureIDInput.InputName = FName("FeatureID");
  FeatureIDInput.Input.Expression = GetFeatureExpression;

  GetPropertyValuesFunction->AdditionalOutputs.Reserve(
      PropertyTable.Properties.Num());

  FString PropertyTableName = createHlslSafeName(PropertyTable.Name);
  bool foundFirstProperty = false;
  for (const FCesiumPropertyTablePropertyDescription& Property :
       PropertyTable.Properties) {
    if (Property.EncodingDetails.Conversion ==
            ECesiumEncodedMetadataConversion::None ||
        !Property.EncodingDetails.HasValidType()) {
      continue;
    }

    PropertyDataSectionY += Incr;

    FString PropertyName = createHlslSafeName(Property.Name);
    // Example: "roofColor_DATA"
    FString PropertyDataName = PropertyName + MaterialPropertyDataSuffix;

    if (!foundFirstProperty) {
      // Get the dimensions of the first valid property. All the properties
      // will have the same pixel dimensions since it is based on the feature
      // count.
      GetPropertyValuesFunction->Code +=
          "uint _czm_width;\nuint _czm_height;\n";
      GetPropertyValuesFunction->Code +=
          PropertyDataName + ".GetDimensions(_czm_width, _czm_height);\n";
      GetPropertyValuesFunction->Code +=
          "uint _czm_featureIndex = round(FeatureID);\n";
      GetPropertyValuesFunction->Code +=
          "uint _czm_pixelX = _czm_featureIndex % _czm_width;\n";
      GetPropertyValuesFunction->Code +=
          "uint _czm_pixelY = _czm_featureIndex / _czm_width;\n";

      foundFirstProperty = true;
    }

    UMaterialExpressionTextureObjectParameter* PropertyData =
        NewObject<UMaterialExpressionTextureObjectParameter>(
            TargetMaterialLayer);
    FString FullPropertyName = getMaterialNameForPropertyTableProperty(
        PropertyTableName,
        PropertyName);
    PropertyData->ParameterName = FName(FullPropertyName);
    PropertyData->MaterialExpressionEditorX = BeginSectionX;
    PropertyData->MaterialExpressionEditorY = PropertyDataSectionY;
    AutoGeneratedNodes.Add(PropertyData);

    MaximumPropertyDataSectionX = FMath::Max(
        MaximumPropertyDataSectionX,
        Incr * GetNameLengthScalar(PropertyData->ParameterName));

    FCustomInput& PropertyInput =
        GetPropertyValuesFunction->Inputs.Emplace_GetRef();
    PropertyInput.InputName = FName(PropertyDataName);
    PropertyInput.Input.Expression = PropertyData;

    FCustomOutput& PropertyOutput =
        GetPropertyValuesFunction->AdditionalOutputs.Emplace_GetRef();

    FString OutputName = PropertyName;
    if (Property.PropertyDetails.bIsNormalized ||
        Property.PropertyDetails.bHasOffset ||
        Property.PropertyDetails.bHasScale) {
      OutputName += MaterialPropertyRawSuffix;
    }

    PropertyOutput.OutputName = FName(OutputName);
    GetPropertyValuesFunction->Outputs.Add(
        FExpressionOutput(PropertyOutput.OutputName));

    FString swizzle = GetSwizzleForEncodedType(Property.EncodingDetails.Type);
    PropertyOutput.OutputType =
        GetOutputTypeForEncodedType(Property.EncodingDetails.Type);

    FString asComponentString =
        Property.EncodingDetails.ComponentType ==
                ECesiumEncodedMetadataComponentType::Float
            ? "asfloat"
            : "asuint";

    // Example:
    // "color = asfloat(color_DATA.Load(int3(_czm_pixelX, _czm_pixelY,
    // 0)).rgb);"
    GetPropertyValuesFunction->Code +=
        OutputName + " = " + asComponentString + "(" + PropertyDataName +
        ".Load(int3(_czm_pixelX, _czm_pixelY, 0))" + swizzle + ");\n";

    if (Property.PropertyDetails.HasValueTransforms()) {
      int32 PropertyTransformsSectionX =
          0.25 * Incr + GetPropertyValuesFunctionWidth;
      GenerateNodesForMetadataPropertyTransforms(
          Property.PropertyDetails,
          Property.EncodingDetails.Type,
          PropertyName,
          FullPropertyName,
          PropertyTransformNodes,
          TargetMaterialLayer,
          PropertyTransformsSectionX,
          PropertyTransformsSectionY,
          GetPropertyValuesFunction,
          GetPropertyValuesFunction->Outputs.Num() - 1);

      MaximumPropertyTransformsSectionX = FMath::Max(
          MaximumPropertyTransformsSectionX,
          PropertyTransformsSectionX);
    }
  }

  // Shift the X of GetPropertyValues depending on the width of the data
  // parameters.
  GetPropertyValuesFunction->MaterialExpressionEditorX +=
      MaximumPropertyDataSectionX + Incr;
  NodeX = GetPropertyValuesFunction->MaterialExpressionEditorX +
          GetPropertyValuesFunctionWidth;

  // Reposition all of the nodes related to property transforms.
  for (UMaterialExpression* Node : PropertyTransformNodes) {
    Node->MaterialExpressionEditorX +=
        GetPropertyValuesFunction->MaterialExpressionEditorX;
    AutoGeneratedNodes.Add(Node);
  }

  // Return the feature ID.
  GetPropertyValuesFunction->OutputType =
      ECustomMaterialOutputType::CMOT_Float1;
  GetPropertyValuesFunction->Code += "return FeatureID;";

  NodeX = GetPropertyValuesFunction->MaterialExpressionEditorX +
          GetPropertyValuesFunctionWidth + MaximumPropertyTransformsSectionX +
          Incr;
  NodeY = FMath::Max(PropertyDataSectionY, PropertyTransformsSectionY) + Incr;
}

/**
 * @brief Generates the nodes necessary to retrieve values from a property
 * texture. In summary:
 * - Gets UVs from primitive with SelectTexCoords (if applicable)
 * - Creates GetPropertyValuesFrom function for property texture
 * - Adds parameter nodes for each property's UV index, data, and channels array
 * - Creates nodes to handle property transforms (if applicable)
 */
void GenerateNodesForPropertyTexture(
    const FCesiumPropertyTextureDescription& PropertyTexture,
    TArray<UMaterialExpression*>& AutoGeneratedNodes,
    UMaterialFunctionMaterialLayer* TargetMaterialLayer,
    const MaterialFunctionLibrary& FunctionLibrary,
    int32& NodeX,
    int32& NodeY,
    bool bHasTexCoords) {
  int32 BeginSectionX = NodeX;
  // This value is used by parameters on the left side of the
  // "GetPropertyValues" function...
  int32 PropertyDataSectionY = NodeY;
  // ...whereas this value is used for parameters on the right side of the
  // function.
  int32 PropertyTransformsSectionY = NodeY + 20;

  UMaterialExpressionCustom* GetPropertyValuesFunction =
      NewObject<UMaterialExpressionCustom>(TargetMaterialLayer);
  GetPropertyValuesFunction->Inputs.Reset(3 * PropertyTexture.Properties.Num());
  GetPropertyValuesFunction->Outputs.Reset(
      PropertyTexture.Properties.Num() + 1);
  GetPropertyValuesFunction->Outputs.Add(FExpressionOutput(TEXT("return")));
  GetPropertyValuesFunction->bShowOutputNameOnPin = true;
  GetPropertyValuesFunction->Code = "";
  GetPropertyValuesFunction->Description =
      GetPropertyValuesPrefix + PropertyTexture.Name;
  GetPropertyValuesFunction->MaterialExpressionEditorX = NodeX;
  GetPropertyValuesFunction->MaterialExpressionEditorY = NodeY;
  AutoGeneratedNodes.Add(GetPropertyValuesFunction);

  int32 GetPropertyValuesFunctionWidth =
      Incr * GetNameLengthScalar(GetPropertyValuesFunction->Description);

  // To prevent nodes from overlapping -- especially if they have really long
  // names -- the GetPropertyValuesFunction node will be shifted to the right
  // depending on the longest name among the parameters on the left.
  int32 MaximumPropertyDataSectionX = 0;
  // In a similar vein, this tracks the overall width of the property transforms
  // section. This will be added to NodeX at the end so that nodes can continue
  // to spawn horizontally.
  int32 MaximumPropertyTransformsSectionX = 0;

  // The nodes to the right of GetPropertyValuesFunction will also need to be
  // shifted, hence this array to keep track of them.
  TArray<UMaterialExpression*> PropertyTransformNodes;

  FString PropertyTextureName = createHlslSafeName(PropertyTexture.Name);
  bool foundFirstProperty = false;

  for (const FCesiumPropertyTexturePropertyDescription& Property :
       PropertyTexture.Properties) {
    if (!isSupportedPropertyTextureProperty(Property.PropertyDetails)) {
      // Ignore properties that are unsupported, i.e., properties that require
      // more than four bytes to parse values from. This limitation is imposed
      // by cesium-native because only single-byte channels are supported.
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "Skipping material node generation for unsupported property texture property %s in %s."),
          *Property.Name,
          *PropertyTexture.Name);
      continue;
    }

    FString PropertyName = createHlslSafeName(Property.Name);
    FString FullPropertyName = getMaterialNameForPropertyTextureProperty(
        PropertyTextureName,
        PropertyName);
    ECesiumEncodedMetadataType Type =
        CesiumMetadataPropertyDetailsToEncodingDetails(Property.PropertyDetails)
            .Type;

    if (!foundFirstProperty) {
      // Define this helper function at the beginning of the code. This extracts
      // the correct value from a float4 based on the given channel index.
      // This is needed because the code input[index] doesn't seem to work with
      // a dynamic index.
      FString StructName =
          MaterialPropertyTexturePrefix + PropertyTextureName + "Functions";
      // clang-format off
      GetPropertyValuesFunction->Code +=
          "struct " + StructName + " {\n "
          "  float Get(float4 input, uint index) {\n"
          "    switch (index) {\n"
          "      case 0:\n    return input.r;\n"
          "      case 1:\n    return input.g;\n"
          "      case 2:\n    return input.b;\n"
          "      case 3:\n    return input.a;\n"
          "      default:\n    return 0.0f;\n"
          "    }\n"
          "  }\n"
          "};\n" +
          StructName + " f;\n";
      // clang-format on

      // Also declare some temporary variables for later use.
      GetPropertyValuesFunction->Code +=
          "float4 sampleColor = float4(0, 0, 0, 0);\n"
          "uint byteOffset = 0;\n"
          "uint sample = 0;\n"
          "uint channel = 0;\n\n";

      foundFirstProperty = true;
    }

    UMaterialExpressionMaterialFunctionCall* TexCoordsInputFunction = nullptr;

    if (bHasTexCoords) {
      UMaterialExpressionScalarParameter* TexCoordsIndex =
          NewObject<UMaterialExpressionScalarParameter>(TargetMaterialLayer);
      TexCoordsIndex->ParameterName =
          FName(FullPropertyName + MaterialTexCoordIndexSuffix);
      TexCoordsIndex->DefaultValue = 0.0f;
      TexCoordsIndex->MaterialExpressionEditorY = NodeX;
      TexCoordsIndex->MaterialExpressionEditorY = PropertyDataSectionY;
      AutoGeneratedNodes.Add(TexCoordsIndex);

      NodeX +=
          Incr * (GetNameLengthScalar(TexCoordsIndex->ParameterName) + 0.2f);

      UMaterialExpressionMaterialFunctionCall* SelectTexCoords =
          NewObject<UMaterialExpressionMaterialFunctionCall>(
              TargetMaterialLayer);
      SelectTexCoords->MaterialFunction = FunctionLibrary.SelectTexCoords;
      SelectTexCoords->MaterialExpressionEditorX = NodeX;
      SelectTexCoords->MaterialExpressionEditorY = PropertyDataSectionY;

      FunctionLibrary.SelectTexCoords->GetInputsAndOutputs(
          SelectTexCoords->FunctionInputs,
          SelectTexCoords->FunctionOutputs);
      SelectTexCoords->FunctionInputs[0].Input.Expression = TexCoordsIndex;
      AutoGeneratedNodes.Add(SelectTexCoords);
      TexCoordsInputFunction = SelectTexCoords;

      MaximumPropertyDataSectionX = NodeX + 2 * Incr;
      NodeX = BeginSectionX;

      if (Property.bHasKhrTextureTransform) {
        PropertyDataSectionY += 1.25 * Incr;

        UMaterialExpressionVectorParameter* TransformRotation =
            NewObject<UMaterialExpressionVectorParameter>(TargetMaterialLayer);
        TransformRotation->ParameterName =
            FName(FullPropertyName + MaterialTextureRotationSuffix);
        TransformRotation->DefaultValue = {0, 1, 0, 1};
        TransformRotation->MaterialExpressionEditorX = NodeX;
        TransformRotation->MaterialExpressionEditorY = PropertyDataSectionY;
        AutoGeneratedNodes.Add(TransformRotation);

        UMaterialExpressionVectorParameter* TransformScaleOffset =
            NewObject<UMaterialExpressionVectorParameter>(TargetMaterialLayer);
        TransformScaleOffset->ParameterName =
            FName(FullPropertyName + MaterialTextureScaleOffsetSuffix);
        TransformScaleOffset->DefaultValue = {1, 1, 0, 0};
        TransformScaleOffset->MaterialExpressionEditorX = NodeX;
        TransformScaleOffset->MaterialExpressionEditorY =
            PropertyDataSectionY + Incr;
        AutoGeneratedNodes.Add(TransformScaleOffset);

        UMaterialExpressionAppendVector* AppendScale =
            NewObject<UMaterialExpressionAppendVector>(TargetMaterialLayer);
        AppendScale->MaterialExpressionEditorX =
            NodeX + Incr * (0.5 + GetNameLengthScalar(
                                      TransformScaleOffset->ParameterName));
        AppendScale->MaterialExpressionEditorY =
            TransformRotation->MaterialExpressionEditorY;
        AppendScale->A.Connect(1, TransformScaleOffset);
        AppendScale->B.Connect(2, TransformScaleOffset);
        AutoGeneratedNodes.Add(AppendScale);

        UMaterialExpressionAppendVector* AppendOffset =
            NewObject<UMaterialExpressionAppendVector>(TargetMaterialLayer);
        AppendOffset->MaterialExpressionEditorX =
            AppendScale->MaterialExpressionEditorX;
        AppendOffset->MaterialExpressionEditorY =
            TransformScaleOffset->MaterialExpressionEditorY;
        AppendOffset->A.Connect(3, TransformScaleOffset);
        AppendOffset->B.Connect(4, TransformScaleOffset);
        AutoGeneratedNodes.Add(AppendOffset);

        MaximumPropertyDataSectionX = FMath::Max(
            MaximumPropertyDataSectionX,
            AppendOffset->MaterialExpressionEditorX + Incr - NodeX);
        PropertyDataSectionY += 1.25 * Incr;

        UMaterialExpressionMaterialFunctionCall* TransformTexCoords =
            NewObject<UMaterialExpressionMaterialFunctionCall>(
                TargetMaterialLayer);
        TransformTexCoords->MaterialFunction =
            FunctionLibrary.TransformTexCoords;
        TransformTexCoords->MaterialExpressionEditorX =
            SelectTexCoords->MaterialExpressionEditorX + Incr * 1.5;
        TransformTexCoords->MaterialExpressionEditorY =
            SelectTexCoords->MaterialExpressionEditorY;

        FunctionLibrary.TransformTexCoords->GetInputsAndOutputs(
            TransformTexCoords->FunctionInputs,
            TransformTexCoords->FunctionOutputs);
        // For some reason, Connect() doesn't work with this input...
        TransformTexCoords->FunctionInputs[0].Input.Expression =
            SelectTexCoords;
        TransformTexCoords->FunctionInputs[0].Input.OutputIndex = 0;
        TransformTexCoords->FunctionInputs[1].Input.Connect(
            0,
            TransformRotation);
        TransformTexCoords->FunctionInputs[2].Input.Connect(0, AppendScale);
        TransformTexCoords->FunctionInputs[3].Input.Connect(0, AppendOffset);
        AutoGeneratedNodes.Add(TransformTexCoords);

        TexCoordsInputFunction = TransformTexCoords;

        MaximumPropertyDataSectionX = FMath::Max(
            MaximumPropertyDataSectionX,
            TransformTexCoords->MaterialExpressionEditorX + Incr * 1.5);
      }

      PropertyDataSectionY += 0.8 * Incr;
    }

    UMaterialExpressionTextureObjectParameter* PropertyData =
        NewObject<UMaterialExpressionTextureObjectParameter>(
            TargetMaterialLayer);
    PropertyData->ParameterName = FName(FullPropertyName);
    PropertyData->MaterialExpressionEditorX = NodeX;
    PropertyData->MaterialExpressionEditorY = PropertyDataSectionY;
    AutoGeneratedNodes.Add(PropertyData);

    MaximumPropertyDataSectionX = FMath::Max(
        MaximumPropertyDataSectionX,
        Incr * GetNameLengthScalar(PropertyData->ParameterName));
    PropertyDataSectionY += Incr;

    UMaterialExpressionVectorParameter* Channels =
        NewObject<UMaterialExpressionVectorParameter>(TargetMaterialLayer);
    Channels->ParameterName = FName(FullPropertyName + MaterialChannelsSuffix);
    Channels->DefaultValue = FLinearColor(0, 0, 0, 0);
    Channels->MaterialExpressionEditorX = NodeX;
    Channels->MaterialExpressionEditorY = PropertyDataSectionY;
    AutoGeneratedNodes.Add(Channels);

    UMaterialExpressionAppendVector* AppendChannels =
        NewObject<UMaterialExpressionAppendVector>(TargetMaterialLayer);
    AppendChannels->MaterialExpressionEditorX =
        NodeX + Incr * (1 + GetNameLengthScalar(Channels->ParameterName));
    AppendChannels->MaterialExpressionEditorY = PropertyDataSectionY;
    AppendChannels->A.Connect(0, Channels);
    AppendChannels->B.Connect(4, Channels);
    AutoGeneratedNodes.Add(AppendChannels);

    MaximumPropertyDataSectionX = FMath::Max(
        MaximumPropertyDataSectionX,
        Incr * GetNameLengthScalar(Channels->ParameterName));

    FCustomInput& TexCoordsInput =
        GetPropertyValuesFunction->Inputs.Emplace_GetRef();
    FString PropertyTextureUVName = PropertyName + MaterialPropertyUVSuffix;
    TexCoordsInput.InputName = FName(PropertyTextureUVName);
    TexCoordsInput.Input.Expression = TexCoordsInputFunction;

    FCustomInput& PropertyTextureInput =
        GetPropertyValuesFunction->Inputs.Emplace_GetRef();
    FString PropertyTextureDataName = PropertyName + MaterialPropertyDataSuffix;
    PropertyTextureInput.InputName = FName(PropertyTextureDataName);
    PropertyTextureInput.Input.Expression = PropertyData;

    FCustomInput& ChannelsInput =
        GetPropertyValuesFunction->Inputs.Emplace_GetRef();
    FString PropertyTextureChannelsName = PropertyName + MaterialChannelsSuffix;
    ChannelsInput.InputName = FName(PropertyTextureChannelsName);
    ChannelsInput.Input.Expression = AppendChannels;

    FCustomOutput& PropertyOutput =
        GetPropertyValuesFunction->AdditionalOutputs.Emplace_GetRef();

    FString OutputName = PropertyName;
    if (Property.PropertyDetails.bIsNormalized ||
        Property.PropertyDetails.bHasOffset ||
        Property.PropertyDetails.bHasScale) {
      OutputName += MaterialPropertyRawSuffix;
    }

    PropertyOutput.OutputName = FName(OutputName);
    GetPropertyValuesFunction->Outputs.Add(
        FExpressionOutput(PropertyOutput.OutputName));

    PropertyOutput.OutputType = GetOutputTypeForEncodedType(Type);

    GetPropertyValuesFunction->Code += GenerateCodeForPropertyTextureProperty(
        PropertyName,
        PropertyTextureUVName,
        PropertyTextureDataName,
        PropertyTextureChannelsName,
        Property.PropertyDetails);

    if (Property.PropertyDetails.HasValueTransforms()) {
      int32 PropertyTransformsSectionX =
          0.2f * Incr + GetPropertyValuesFunctionWidth;
      GenerateNodesForMetadataPropertyTransforms(
          Property.PropertyDetails,
          Type,
          PropertyName,
          FullPropertyName,
          PropertyTransformNodes,
          TargetMaterialLayer,
          PropertyTransformsSectionX,
          PropertyTransformsSectionY,
          GetPropertyValuesFunction,
          GetPropertyValuesFunction->Outputs.Num() - 1);

      MaximumPropertyTransformsSectionX = FMath::Max(
          MaximumPropertyTransformsSectionX,
          PropertyTransformsSectionX);
    }

    PropertyDataSectionY += Incr;
  }

  // Set the X of GetPropertyValues.
  GetPropertyValuesFunction->MaterialExpressionEditorX +=
      MaximumPropertyDataSectionX + Incr;

  // Reposition all of the nodes related to property transforms.
  for (UMaterialExpression* Node : PropertyTransformNodes) {
    Node->MaterialExpressionEditorX +=
        GetPropertyValuesFunction->MaterialExpressionEditorX;
    AutoGeneratedNodes.Add(Node);
  }

  // Obligatory return code.
  GetPropertyValuesFunction->OutputType =
      ECustomMaterialOutputType::CMOT_Float1;
  GetPropertyValuesFunction->Code += "return 0.0f;";

  NodeX = GetPropertyValuesFunction->MaterialExpressionEditorX +
          GetPropertyValuesFunctionWidth + MaximumPropertyTransformsSectionX +
          Incr;
  NodeY = FMath::Max(PropertyDataSectionY, PropertyTransformsSectionY) + Incr;
}

UMaterialExpression* GenerateInstanceNodes(
    TArray<UMaterialExpression*>& AutoGeneratedNodes,
    UMaterialFunctionMaterialLayer* TargetMaterialLayer,
    UMaterialFunction* GetFeatureIdsFromInstanceFunction,
    int32& NodeX,
    int32& NodeY) {
  UMaterialExpressionMaterialFunctionCall* GetFeatureIds =
      NewObject<UMaterialExpressionMaterialFunctionCall>(TargetMaterialLayer);
  GetFeatureIds->MaterialFunction = GetFeatureIdsFromInstanceFunction;
  GetFeatureIds->MaterialExpressionEditorX = NodeX;
  GetFeatureIds->MaterialExpressionEditorY = NodeY;

  GetFeatureIdsFromInstanceFunction->GetInputsAndOutputs(
      GetFeatureIds->FunctionInputs,
      GetFeatureIds->FunctionOutputs);

  NodeX += 2 * Incr;
  AutoGeneratedNodes.Add(GetFeatureIds);
  return GetFeatureIds;
}

void GenerateMaterialNodes(
    UCesiumFeaturesMetadataComponent* pComponent,
    MaterialGenerationState& MaterialState,
    const MaterialFunctionLibrary& FunctionLibrary) {
  const TArray<FCesiumFeatureIdSetDescription>& FeatureIdSets =
      pComponent->Description.PrimitiveFeatures.FeatureIdSets;
  const TArray<FCesiumPropertyTableDescription>& PropertyTables =
      pComponent->Description.ModelMetadata.PropertyTables;
  const TArray<FCesiumPropertyTextureDescription>& PropertyTextures =
      pComponent->Description.ModelMetadata.PropertyTextures;
  const TSet<FString> PropertyTextureNames =
      pComponent->Description.PrimitiveMetadata.PropertyTextureNames;

  int32 NodeX = 0;
  int32 NodeY = 0;

  int32 BeginSectionX = NodeX;
  int32 MaximumSectionX = BeginSectionX;

  TSet<FString> GeneratedPropertyTableNames;
  GeneratedPropertyTableNames.Reserve(PropertyTables.Num());

  for (const FCesiumFeatureIdSetDescription& featureIdSet : FeatureIdSets) {
    if (featureIdSet.Type == ECesiumFeatureIdSetType::None) {
      continue;
    }

    UMaterialExpressionMaterialFunctionCall* GetFeatureIdCall = nullptr;
    UMaterialExpression* LastNode = nullptr;
    if (featureIdSet.Type == ECesiumFeatureIdSetType::Texture) {
      LastNode = GenerateNodesForFeatureIdTexture(
          featureIdSet,
          MaterialState.AutoGeneratedNodes,
          pComponent->TargetMaterialLayer,
          FunctionLibrary,
          NodeX,
          NodeY);
    } else if (featureIdSet.Type == ECesiumFeatureIdSetType::Instance) {
      LastNode = GenerateInstanceNodes(
          MaterialState.AutoGeneratedNodes,
          pComponent->TargetMaterialLayer,
          FunctionLibrary.GetFeatureIdsFromInstance,
          NodeX,
          NodeY);
    } else {
      // Handle implicit feature IDs the same as feature ID attributes
      LastNode = GenerateNodesForFeatureIdAttribute(
          featureIdSet,
          MaterialState.AutoGeneratedNodes,
          pComponent->TargetMaterialLayer,
          FunctionLibrary.GetFeatureIdsFromAttribute,
          NodeX,
          NodeY);
    }

    int32 BeginSectionY = NodeY;

    if (!featureIdSet.PropertyTableName.IsEmpty()) {
      const FCesiumPropertyTableDescription* pPropertyTable =
          PropertyTables.FindByPredicate(
              [&name = featureIdSet.PropertyTableName](
                  const FCesiumPropertyTableDescription&
                      existingPropertyTable) {
                return existingPropertyTable.Name == name;
              });

      if (pPropertyTable) {
        GenerateNodesForPropertyTable(
            *pPropertyTable,
            MaterialState.AutoGeneratedNodes,
            pComponent->TargetMaterialLayer,
            NodeX,
            NodeY,
            LastNode);
        GeneratedPropertyTableNames.Add(pPropertyTable->Name);
      }
    }

    // Spatial nitpicking; this aligns the if statement to the same Y as the
    // PropertyTableFunction node then resets the Y so that the next section
    // appears below all of the just-generated nodes.
    int32 OriginalY = NodeY;
    NodeY = BeginSectionY;

    // Even if a feature ID set doesn't specify a `nullFeatureId`, we use -1
    // as the default.
    GenerateNodesForNullFeatureId(
        featureIdSet,
        MaterialState.AutoGeneratedNodes,
        pComponent->TargetMaterialLayer,
        NodeX,
        NodeY,
        LastNode);

    NodeY = OriginalY;
    MaximumSectionX = FMath::Max(MaximumSectionX, NodeX);

    NodeX = BeginSectionX;
    NodeY += 1.75 * Incr;
  }

  // Generate nodes for any property tables that aren't linked to a feature ID
  // set.
  for (const FCesiumPropertyTableDescription& propertyTable : PropertyTables) {
    if (!GeneratedPropertyTableNames.Find(propertyTable.Name)) {
      GenerateNodesForPropertyTable(
          propertyTable,
          MaterialState.AutoGeneratedNodes,
          pComponent->TargetMaterialLayer,
          NodeX,
          NodeY,
          nullptr);
      MaximumSectionX = FMath::Max(MaximumSectionX, NodeX);

      NodeX = BeginSectionX;
      NodeY += 1.75 * Incr;
    }
  }

  NodeY += Incr;
  NodeX = BeginSectionX;

  TSet<FString> GeneratedPropertyTextureNames;
  GeneratedPropertyTextureNames.Reserve(PropertyTextures.Num());

  for (const FString& propertyTextureName : PropertyTextureNames) {
    const FCesiumPropertyTextureDescription* pPropertyTexture =
        PropertyTextures.FindByPredicate(
            [&propertyTextureName](const FCesiumPropertyTextureDescription&
                                       existingPropertyTexture) {
              return existingPropertyTexture.Name == propertyTextureName;
            });
    if (!pPropertyTexture) {
      continue;
    }

    GenerateNodesForPropertyTexture(
        *pPropertyTexture,
        MaterialState.AutoGeneratedNodes,
        pComponent->TargetMaterialLayer,
        FunctionLibrary,
        NodeX,
        NodeY,
        true);
    GeneratedPropertyTextureNames.Add(propertyTextureName);

    MaximumSectionX = FMath::Max(MaximumSectionX, NodeX);

    NodeY += 1.75 * Incr;
    NodeX = BeginSectionX;
  }

  // Generate nodes for any property textures that aren't linked to a
  // primitive / texture coordinate set.
  for (const FCesiumPropertyTextureDescription& propertyTexture :
       PropertyTextures) {
    if (!GeneratedPropertyTextureNames.Find(propertyTexture.Name)) {
      GenerateNodesForPropertyTexture(
          propertyTexture,
          MaterialState.AutoGeneratedNodes,
          pComponent->TargetMaterialLayer,
          FunctionLibrary,
          NodeX,
          NodeY,
          false);

      MaximumSectionX = FMath::Max(MaximumSectionX, NodeX);

      NodeY += 2 * Incr;
      NodeX = BeginSectionX;
    }
  }

  NodeY = -2 * Incr;

  UMaterialExpressionFunctionInput* InputMaterial = nullptr;
  for (const TObjectPtr<UMaterialExpression>& ExistingNode :
       pComponent->TargetMaterialLayer->GetExpressionCollection().Expressions) {
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
    MaterialState.OneTimeGeneratedNodes.Add(InputMaterial);
  }

  NodeX += BeginSectionX + MaximumSectionX;

  UMaterialExpressionSetMaterialAttributes* SetMaterialAttributes = nullptr;
  for (const TObjectPtr<UMaterialExpression>& ExistingNode :
       pComponent->TargetMaterialLayer->GetExpressionCollection().Expressions) {
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
    MaterialState.OneTimeGeneratedNodes.Add(SetMaterialAttributes);
  }

  SetMaterialAttributes->Inputs[0].Expression = InputMaterial;
  SetMaterialAttributes->MaterialExpressionEditorX = NodeX;
  SetMaterialAttributes->MaterialExpressionEditorY = NodeY;

  NodeX += 2 * Incr;

  UMaterialExpressionFunctionOutput* OutputMaterial = nullptr;
  for (const TObjectPtr<UMaterialExpression>& ExistingNode :
       pComponent->TargetMaterialLayer->GetExpressionCollection().Expressions) {
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
    MaterialState.OneTimeGeneratedNodes.Add(OutputMaterial);
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

  const FString MaterialName =
      "ML_" + pTileset->GetFName().ToString() + "_FeaturesMetadata";
  const FString PackageBaseName = "/Game/";
  const FString PackageName = PackageBaseName + MaterialName;

  MaterialFunctionLibrary FunctionLibrary = MaterialFunctionLibrary();
  if (!FunctionLibrary.isValid()) {
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
    this->TargetMaterialLayer = CreateMaterialLayer(PackageName, MaterialName);
  }

  this->TargetMaterialLayer->PreEditChange(NULL);

  MaterialGenerationState MaterialState;

  ClearAutoGeneratedNodes(
      this->TargetMaterialLayer,
      MaterialState.ConnectionInputRemap,
      MaterialState.ConnectionOutputRemap,
      FunctionLibrary);

  GenerateMaterialNodes(this, MaterialState, FunctionLibrary);
  MoveNodesToMaterialLayer(MaterialState, this->TargetMaterialLayer);

  RemapUserConnections(
      this->TargetMaterialLayer,
      MaterialState.ConnectionInputRemap,
      MaterialState.ConnectionOutputRemap,
      FunctionLibrary);

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
      GEngine->EndTransaction();
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

void UCesiumFeaturesMetadataComponent::PostLoad() {
  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  // These deprecated variables should only be non-empty on the first load, in
  // which case the contents of Description should be empty.
  if (this->FeatureIdSets.Num() > 0) {
    CESIUM_ASSERT(this->Description.PrimitiveFeatures.FeatureIdSets.Num == 0);
    Swap(
        this->FeatureIdSets,
        this->Description.PrimitiveFeatures.FeatureIdSets);
  }
  if (this->PropertyTextureNames.Num() > 0) {
    CESIUM_ASSERT(
        this->Description.PrimitiveMetadata.PropertyTextureNames.Num == 0);
    Swap(
        this->PropertyTextureNames,
        this->Description.PrimitiveMetadata.PropertyTextureNames);
  }
  if (this->PropertyTables.Num() > 0) {
    CESIUM_ASSERT(this->Description.ModelMetadata.PropertyTables.Num == 0);
    Swap(this->PropertyTables, this->Description.ModelMetadata.PropertyTables);
  }
  if (this->PropertyTextures.Num() > 0) {
    CESIUM_ASSERT(this->Description.ModelMetadata.PropertyTextures.Num == 0);
    Swap(
        this->PropertyTextures,
        this->Description.ModelMetadata.PropertyTextures);
  }
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  Super::PostLoad();
}
