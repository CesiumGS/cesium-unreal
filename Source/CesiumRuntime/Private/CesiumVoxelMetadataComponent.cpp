// Copyright 2020-2026 CesiumGS, Inc. and Contributors

#include "CesiumVoxelMetadataComponent.h"

#include "Cesium3DTileset.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumModelMetadata.h"
#include "CesiumRuntime.h"
#include "CesiumVoxelRendererComponent.h"
#include "EncodedFeaturesMetadata.h"
#include "EncodedMetadataConversions.h"
#include "GenerateMaterialUtility.h"
#include "Misc/FileHelper.h"
#include "ShaderCore.h"
#include "UnrealMetadataConversions.h"

#include <Cesium3DTiles/Class.h>
#include <Cesium3DTiles/ExtensionContent3dTilesContentVoxels.h>

#if WITH_EDITOR
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ComponentReregisterContext.h"
#include "Containers/LazyPrintf.h"
#include "Containers/Map.h"
#include "ContentBrowserModule.h"
#include "Factories/MaterialFunctionMaterialLayerFactory.h"
#include "IContentBrowserSingleton.h"
#include "IMaterialEditor.h"
#include "Interfaces/IPluginManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialAttributeDefinitionMap.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Materials/MaterialExpressionIf.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionTextureProperty.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/Package.h"
#endif

using namespace EncodedFeaturesMetadata;
using namespace GenerateMaterialUtility;

static const FString RaymarchDescription = "Voxel Raymarch";

UCesiumVoxelMetadataComponent::UCesiumVoxelMetadataComponent() {
  // Structure to hold one-time initialization
  struct FConstructorStatics {
    ConstructorHelpers::FObjectFinder<UTexture> DefaultVolumeTexture;
    FConstructorStatics()
        : DefaultVolumeTexture(
              TEXT("/Engine/EngineResources/DefaultVolumeTexture")) {}
  };
  static FConstructorStatics ConstructorStatics;
  this->_pDefaultVolumeTexture = ConstructorStatics.DefaultVolumeTexture.Object;
}

void UCesiumVoxelMetadataComponent::OnFetchMetadata(
    ACesium3DTileset* pActor,
    const Cesium3DTilesSelection::TilesetMetadata* pMetadata) {
  if (!IsValid(pActor) || !pMetadata || !pMetadata->statistics) {
    // Tilesets may not contain any statistics...
    return;
  }

  // ...however, if statistics are present, then there must be a schema.
  if (!pMetadata->schema) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Tileset %s has incomplete metadata and cannot sync its statistics with "
            "UCesiumVoxelMetadataComponent."),
        *pActor->GetName());
    return;
  }

  const std::string classId(TCHAR_TO_UTF8(*this->Description.ClassId));
  const Cesium3DTiles::Schema& schema = *pMetadata->schema;
  const Cesium3DTiles::Statistics& statistics = *pMetadata->statistics;

  if (!schema.classes.contains(classId) ||
      !statistics.classes.contains(classId)) {
    return;
  }

  const Cesium3DTiles::Class& voxelClass = schema.classes.at(classId);
  const Cesium3DTiles::ClassStatistics& classStatistics =
      statistics.classes.at(classId);

  for (FCesiumMetadataPropertyStatisticsDescription& propertyStatistics :
       this->Description.Statistics) {
    std::string propertyId = TCHAR_TO_UTF8(*propertyStatistics.Id);

    auto propertyStatisticsIt = classStatistics.properties.find(propertyId);
    if (propertyStatisticsIt != classStatistics.properties.end()) {
      FCesiumMetadataValueType type;
      if (voxelClass.properties.contains(propertyId)) {
        type = FCesiumMetadataValueType::fromClassProperty(
            voxelClass.properties.at(propertyId));
      }

      for (FCesiumMetadataPropertyStatisticValue& statisticValue :
           propertyStatistics.Values) {
        statisticValue.Value = getValueForSemantic(
            propertyStatisticsIt->second,
            type,
            statisticValue.Semantic);
      }
    } else {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "Tileset %s does not contain statistics for property %s from class %s "
              "on UCesiumVoxelMetadataComponent."),
          *pActor->GetName(),
          *propertyStatistics.Id,
          *this->Description.ClassId);

      for (FCesiumMetadataPropertyStatisticValue& statisticValue :
           propertyStatistics.Values) {
        statisticValue.Value = FCesiumMetadataValue();
      }
    }
  }

  if (auto* pVoxelRenderer =
          pActor->GetComponentByClass<UCesiumVoxelRendererComponent>()) {
    pVoxelRenderer->SyncStatistics(this->Description);
  }
}

void UCesiumVoxelMetadataComponent::ClearStatistics() {
  for (FCesiumMetadataPropertyStatisticsDescription& propertyStatistics :
       this->Description.Statistics) {
    for (FCesiumMetadataPropertyStatisticValue& statisticValue :
         propertyStatistics.Values) {
      statisticValue.Value = FCesiumMetadataValue();
    }
  }
}

#if WITH_EDITOR
void UCesiumVoxelMetadataComponent::BuildShader() {
  ACesium3DTileset* pOwner = this->GetOwner<ACesium3DTileset>();
  OnCesiumVoxelMetadataBuildShader.Broadcast(pOwner);
}

namespace {
struct VoxelMetadataClassification : public MaterialNodeClassification {
  UMaterialExpressionCustom* RaymarchNode = nullptr;
  UMaterialExpressionMaterialFunctionCall* BreakFloat4Node = nullptr;
};

struct MaterialResourceLibrary {
  FString ShaderTemplate;
  UMaterial* pMaterialTemplate;
  TObjectPtr<UTexture> pDefaultVolumeTexture;

  MaterialResourceLibrary() {
    FString Path = GetShaderSourceFilePath(
        "/Plugin/CesiumForUnreal/Private/CesiumVoxelTemplate.usf");

    if (!Path.IsEmpty()) {
      FFileHelper::LoadFileToString(ShaderTemplate, *Path);
    }

    pMaterialTemplate = LoadObjFromPath<UMaterial>(
        "/CesiumForUnreal/Materials/M_CesiumVoxelMaterial");
  }

  bool isValid() const {
    return !ShaderTemplate.IsEmpty() && IsValid(pMaterialTemplate) &&
           IsValid(pDefaultVolumeTexture);
  }
};

/**
 * Utility for filling CesiumVoxelTemplate.hlsl with necessary code / parameters
 * for styling voxels in an Unreal material.
 */
struct CustomShaderBuilder {
  FString DeclareShaderProperties;
  FString SamplePropertiesFromTexture;
  FString DeclareDataTextureVariables;
  FString SetDataTextures;
  FString SetStatistics;

  /**
   * Declares the property in the CustomShaderProperties struct for use in the
   * shader.
   */
  void AddPropertyDeclaration(
      const FString& PropertyName,
      const FCesiumPropertyAttributePropertyDescription& Property) {
    if (!DeclareShaderProperties.IsEmpty()) {
      DeclareShaderProperties += "\n";
    }

    FString encodedHlslType = GetHlslTypeForEncodedType(
        Property.EncodingDetails.Type,
        Property.EncodingDetails.ComponentType);
    FString normalizedHlslType = FString();

    bool isNormalizedProperty = Property.PropertyDetails.bIsNormalized;

    if (isNormalizedProperty) {
      normalizedHlslType = GetHlslTypeForEncodedType(
          Property.EncodingDetails.Type,
          ECesiumEncodedMetadataComponentType::Float);
      FString rawPropertyName = PropertyName + MaterialPropertyRawSuffix;

      // If the property is normalized, the encoded type actually corresponds to
      // the raw data values. A second line for the normalized value is added.
      // e.g., "uint8 myProperty_RAW; float myProperty;"
      DeclareShaderProperties.Appendf(
          TEXT("\t%s %s;\n\t%s = %s;"),
          *encodedHlslType,
          *rawPropertyName,
          *normalizedHlslType,
          *PropertyName);
    } else {
      // e.g., "float temperature;"
      DeclareShaderProperties.Appendf(
          TEXT("\t%s %s;"),
          *encodedHlslType,
          *PropertyName);
    }

    if (Property.PropertyDetails.bHasNoDataValue) {
      // Expose "no data" value to the shader so the user can act on it.
      // "No data" values are always given in the raw value type.
      FString NoDataName = PropertyName + MaterialPropertyNoDataSuffix;
      DeclareShaderProperties.Appendf(
          TEXT("\n\t%s %s;"),
          *encodedHlslType,
          *NoDataName);
    }

    if (Property.PropertyDetails.bHasDefaultValue) {
      // Expose default value to the shader so the user can act on it.
      FString DefaultValueName =
          PropertyName + MaterialPropertyDefaultValueSuffix;
      DeclareShaderProperties.Appendf(
          TEXT("\n\t%s %s;"),
          *(isNormalizedProperty ? normalizedHlslType : encodedHlslType),
          *DefaultValueName);
    }
  }

  /**
   * Declares the texture parameter in the VoxelDataTextures struct for use in
   * the shader.
   */
  void AddDataTexture(
      const FString& PropertyName,
      const FString& TextureParameterName) {
    if (!DeclareDataTextureVariables.IsEmpty()) {
      DeclareDataTextureVariables += "\n\t";
    }
    // e.g., "Texture3D temperature;"
    DeclareDataTextureVariables.Appendf(TEXT("Texture3D %s;"), *PropertyName);

    if (!SetDataTextures.IsEmpty()) {
      SetDataTextures += "\n";
    }
    // e.g., "DataTextures.temperature = temperature_DATA;"
    SetDataTextures.Appendf(
        TEXT("DataTextures.%s = %s;"),
        *PropertyName,
        *TextureParameterName);
  }

  /**
   * Adds code for correctly retrieving the property from the VoxelDataTextures.
   * Also adds and applies any value transforms in the property.
   */
  void AddPropertyRetrieval(
      const FString& PropertyName,
      const FCesiumPropertyAttributePropertyDescription& Property) {
    if (!SamplePropertiesFromTexture.IsEmpty()) {
      SamplePropertiesFromTexture += "\n\t\t";
    }

    FString encodedHlslType = GetHlslTypeForEncodedType(
        Property.EncodingDetails.Type,
        Property.EncodingDetails.ComponentType);
    FString normalizedHlslType = GetHlslTypeForEncodedType(
        Property.EncodingDetails.Type,
        ECesiumEncodedMetadataComponentType::Float);
    FString rawPropertyName = PropertyName + MaterialPropertyRawSuffix;

    FString swizzle = GetSwizzleForEncodedType(Property.EncodingDetails.Type);
    bool isNormalizedProperty = Property.PropertyDetails.bIsNormalized;
    if (isNormalizedProperty) {
      SamplePropertiesFromTexture.Appendf(
          TEXT("Properties.%s = %s.Load(int4(Coords, 0))%s;"),
          *rawPropertyName,
          *PropertyName,
          *swizzle);
      // Normalization can be hardcoded because only normalized uint8s are
      // supported.
      SamplePropertiesFromTexture.Appendf(
          TEXT("\n\t\tProperties.%s = (Properties.%s / 255.0)"),
          *PropertyName,
          *rawPropertyName);
    } else {
      SamplePropertiesFromTexture.Appendf(
          TEXT("Properties.%s = %s.Load(int4(Coords, 0))%s;"),
          *PropertyName,
          *PropertyName,
          *swizzle);
    }

    if (Property.PropertyDetails.bHasScale) {
      FString ScaleName = PropertyName + MaterialPropertyScaleSuffix;
      // Declare the value transforms underneath the corresponding data texture
      // variable. e.g., float myProperty_SCALE;
      DeclareDataTextureVariables.Appendf(
          TEXT("\n\t%s %s;"),
          *(isNormalizedProperty ? normalizedHlslType : encodedHlslType),
          *ScaleName);
      SetDataTextures.Appendf(
          TEXT("\nDataTextures.%s = %s;"),
          *ScaleName,
          *ScaleName);

      // e.g., " * myProperty_SCALE"
      SamplePropertiesFromTexture += " * " + ScaleName;
    }

    if (Property.PropertyDetails.bHasOffset) {
      FString OffsetName = PropertyName + MaterialPropertyOffsetSuffix;
      DeclareDataTextureVariables.Appendf(
          TEXT("\n\t%s %s;"),
          *(isNormalizedProperty ? normalizedHlslType : encodedHlslType),
          *OffsetName);
      SetDataTextures.Appendf(
          TEXT("\nDataTextures.%s = %s;"),
          *OffsetName,
          *OffsetName);

      // e.g., " + myProperty_OFFSET"
      SamplePropertiesFromTexture += " + " + OffsetName;
    }

    SamplePropertiesFromTexture += ";";

    if (Property.PropertyDetails.bHasNoDataValue) {
      FString NoDataName = PropertyName + MaterialPropertyNoDataSuffix;
      DeclareDataTextureVariables.Appendf(
          TEXT("\n\t%s %s;"),
          *encodedHlslType,
          *NoDataName);
      SetDataTextures.Appendf(
          TEXT("\nDataTextures.%s = %s;"),
          *NoDataName,
          *NoDataName);
      SamplePropertiesFromTexture.Appendf(
          TEXT("\n\t\tProperties.%s = %s;"),
          *NoDataName,
          *NoDataName);
    }

    if (Property.PropertyDetails.bHasDefaultValue) {
      FString DefaultValueName =
          PropertyName + MaterialPropertyDefaultValueSuffix;
      DeclareDataTextureVariables.Appendf(
          TEXT("\n\t%s %s;"),
          *(isNormalizedProperty ? normalizedHlslType : encodedHlslType),
          *DefaultValueName);
      SetDataTextures.Appendf(
          TEXT("\nDataTextures.%s = %s;"),
          *DefaultValueName,
          *DefaultValueName);
      SamplePropertiesFromTexture.Appendf(
          TEXT("\n\t\tProperties.%s = %s;"),
          *DefaultValueName,
          *DefaultValueName);
    }
  }

  /**
   * Declares the statistic in the CustomShaderProperties struct for use in the
   * shader.
   */
  void AddShaderStatistic(
      const FString& PropertyName,
      const FCesiumMetadataPropertyStatisticValue& Statistic) {
    if (!DeclareShaderProperties.IsEmpty()) {
      DeclareShaderProperties += "\n";
    }

    FString statisticName =
        getNameForStatistic(FString(), PropertyName, Statistic.Semantic);
    FCesiumMetadataValueType valueType =
        UCesiumMetadataValueBlueprintLibrary::GetValueType(Statistic.Value);

    FString type = "float";
    switch (valueType.Type) {
    case ECesiumMetadataType::Vec2:
      type += "2";
      break;
    case ECesiumMetadataType::Vec3:
      type += "3";
      break;
    case ECesiumMetadataType::Vec4:
      type += "4";
      break;
    case ECesiumMetadataType::Scalar:
    default:
      break;
    }

    // e.g., "float temperature_MIN;"
    DeclareShaderProperties.Appendf(TEXT("\t%s %s;"), *type, *statisticName);

    if (!SetStatistics.IsEmpty()) {
      SetStatistics += "\n";
    }
    // e.g., "Properties.temperature_MIN = temperature_MIN;
    SetStatistics.Appendf(
        TEXT("Properties.%s = %s;"),
        *statisticName,
        *statisticName);
  }

  /**
   * Comprehensively adds the declaration for properties and data textures, as
   * well as the code to correctly retrieve the property values from the data
   * textures.
   */
  void AddShaderProperty(
      const FString& PropertyName,
      const FString& TextureParameterName,
      const FCesiumPropertyAttributePropertyDescription& Property) {
    AddPropertyDeclaration(PropertyName, Property);
    AddDataTexture(PropertyName, TextureParameterName);
    AddPropertyRetrieval(PropertyName, Property);
  }
};
} // namespace

/*static*/ const FString UCesiumVoxelMetadataComponent::_shaderPreviewTemplate =
    "struct CustomShaderProperties {\n"
    "/* Properties and statistics go here. */\n"
    "%s"
    "\n\n"
    "/* Additional helper functions go here.*/\n"
    "%s\n\n"
    "float4 Shade() {\n"
    "/* Custom shader code goes here.*/\n"
    "%s\n"
    "\t}\n}";

FString UCesiumVoxelMetadataComponent::getCustomShaderPreview() const {
  // Inspired by HLSLMaterialTranslator.cpp. Similar to MaterialTemplate.ush,
  // CesiumVoxelTemplate.ush contains "%s" formatters that should be replaced
  // with generated code.
  FLazyPrintf LazyPrintf(*_shaderPreviewTemplate);
  CustomShaderBuilder Builder;

  for (const FCesiumPropertyAttributePropertyDescription& Property :
       this->Description.Properties) {
    if (!isSupportedPropertyAttributeProperty(Property.PropertyDetails)) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "Property %s (%s) is not supported for voxels and will not be added to the generated material."),
          *Property.Name,
          *FCesiumMetadataValueType(
               Property.PropertyDetails.Type,
               Property.PropertyDetails.ComponentType,
               Property.PropertyDetails.bIsArray)
               .ToString());
      continue;
    }

    FString PropertyName = createHlslSafeName(Property.Name);
    Builder.AddPropertyDeclaration(PropertyName, Property);
  }

  for (const FCesiumMetadataPropertyStatisticsDescription& propertyStatistics :
       this->Description.Statistics) {
    for (const FCesiumMetadataPropertyStatisticValue& statistic :
         propertyStatistics.Values) {
      Builder.AddShaderStatistic(propertyStatistics.Id, statistic);
    }
  }

  LazyPrintf.PushParam(*Builder.DeclareShaderProperties);
  LazyPrintf.PushParam(*this->AdditionalFunctions);
  LazyPrintf.PushParam(*this->CustomShader);

  return LazyPrintf.GetResultString();
}

static VoxelMetadataClassification ClassifyNodes(UMaterial* pMaterial) {
  VoxelMetadataClassification Classification;
  for (const TObjectPtr<UMaterialExpression>& pNode :
       pMaterial->GetExpressionCollection().Expressions) {
    // Check if this node is marked as autogenerated.
    if (pNode->Desc.StartsWith(
            AutogeneratedMessage,
            ESearchCase::Type::CaseSensitive)) {
      Classification.AutoGeneratedNodes.Add(pNode);

      UMaterialExpressionCustom* pCustomNode =
          Cast<UMaterialExpressionCustom>(pNode);
      if (pCustomNode &&
          pCustomNode->Description.Contains(RaymarchDescription)) {
        Classification.RaymarchNode = pCustomNode;
        continue;
      }

      UMaterialExpressionMaterialFunctionCall* pFunctionCall =
          Cast<UMaterialExpressionMaterialFunctionCall>(pNode);
      const FString& FunctionName =
          (pFunctionCall && pFunctionCall->MaterialFunction)
              ? pFunctionCall->MaterialFunction->GetName()
              : FString();
      if (FunctionName.Contains("BreakOutFloat4")) {
        Classification.BreakFloat4Node = pFunctionCall;
      }
    } else {
      Classification.UserAddedNodes.Add(pNode);
    }
  }
  return Classification;
}

static void ClearAutoGeneratedNodes(
    UMaterial* pMaterial,
    TMap<FString, MaterialGenerationState::ParameterConnections>&
        connectionInputMap,
    TMap<FString, TArray<FExpressionInput*>>& connectionOutputMap) {
  VoxelMetadataClassification Classification = ClassifyNodes(pMaterial);
  // Determine which user-added connections to remap when regenerating the
  // voxel raymarch node.
  UMaterialExpressionCustom* pRaymarchNode = Classification.RaymarchNode;
  if (pRaymarchNode && pRaymarchNode->Outputs.Num() > 0) {
    FExpressionOutput& Output = pRaymarchNode->Outputs[0];
    FString Key =
        pRaymarchNode->GetDescription() + Output.OutputName.ToString();

    // Look for user-made connections to this property.
    TArray<FExpressionInput*> Connections;
    for (UMaterialExpression* pUserNode : Classification.UserAddedNodes) {
      for (FExpressionInputIterator It(pUserNode); It; ++It) {
        if (It.Input->Expression == pRaymarchNode &&
            It.Input->OutputIndex == 0) {
          Connections.Add(It.Input);
          It.Input->Expression = nullptr;
        }
      }
    }

    connectionOutputMap.Emplace(MoveTemp(Key), MoveTemp(Connections));
  }

  // Determine which user-added connections to remap when regenerating the
  // break node. This is primarily used to break out the alpha channel, but
  // check all outputs just in case the user has made connections.
  UMaterialExpressionMaterialFunctionCall* pBreakNode =
      Classification.BreakFloat4Node;
  if (pBreakNode) {
    int32 OutputIndex = 0;
    for (const FExpressionOutput& Output : pBreakNode->Outputs) {
      FString Key = pBreakNode->GetDescription() + Output.OutputName.ToString();

      // Look for user-made connections to this property.
      TArray<FExpressionInput*> Connections;
      for (UMaterialExpression* pUserNode : Classification.UserAddedNodes) {
        for (FExpressionInputIterator It(pUserNode); It; ++It) {
          if (It.Input->Expression == pBreakNode &&
              It.Input->OutputIndex == OutputIndex) {
            Connections.Add(It.Input);
            It.Input->Expression = nullptr;
          }
        }
      }

      connectionOutputMap.Emplace(MoveTemp(Key), MoveTemp(Connections));
      ++OutputIndex;
    }
  }

  // Remove auto-generated nodes.
  for (UMaterialExpression* AutoGeneratedNode :
       Classification.AutoGeneratedNodes) {
    pMaterial->GetExpressionCollection().RemoveExpression(AutoGeneratedNode);
  }
}

/**
 * @brief Generates the nodes necessary to apply property transforms to a
 * metadata property.
 */
static void GenerateNodesForMetadataPropertyTransforms(
    UMaterial* pMaterial,
    TArray<UMaterialExpression*>& AutoGeneratedNodes,
    const FCesiumMetadataPropertyDetails& PropertyDetails,
    ECesiumEncodedMetadataType Type,
    const FString& PropertyName,
    int32& NodeX,
    int32& NodeY,
    UMaterialExpressionCustom* RaymarchNode) {
  if (PropertyDetails.bHasScale) {
    NodeY += Incr;
    FString ParameterName = PropertyName + MaterialPropertyScaleSuffix;
    UMaterialExpressionParameter* Parameter =
        GenerateParameterNode(pMaterial, Type, ParameterName, NodeX, NodeY);
    AutoGeneratedNodes.Add(Parameter);

    FCustomInput& ScaleInput = RaymarchNode->Inputs.Emplace_GetRef();
    ScaleInput.InputName = FName(ParameterName);
    ScaleInput.Input.Expression = Parameter;
  }

  if (PropertyDetails.bHasOffset) {
    NodeY += Incr;
    FString ParameterName = PropertyName + MaterialPropertyOffsetSuffix;
    UMaterialExpressionParameter* Parameter =
        GenerateParameterNode(pMaterial, Type, ParameterName, NodeX, NodeY);
    AutoGeneratedNodes.Add(Parameter);

    FCustomInput& OffsetInput = RaymarchNode->Inputs.Emplace_GetRef();
    OffsetInput.InputName = FName(ParameterName);
    OffsetInput.Input.Expression = Parameter;
  }

  if (PropertyDetails.bHasNoDataValue) {
    NodeY += Incr;
    FString ParameterName = PropertyName + MaterialPropertyNoDataSuffix;
    UMaterialExpressionParameter* Parameter =
        GenerateParameterNode(pMaterial, Type, ParameterName, NodeX, NodeY);
    AutoGeneratedNodes.Add(Parameter);

    FCustomInput& NoDataInput = RaymarchNode->Inputs.Emplace_GetRef();
    NoDataInput.InputName = FName(ParameterName);
    NoDataInput.Input.Expression = Parameter;
  }

  if (PropertyDetails.bHasDefaultValue) {
    NodeY += Incr;
    FString ParameterName = PropertyName + MaterialPropertyDefaultValueSuffix;
    UMaterialExpressionParameter* Parameter =
        GenerateParameterNode(pMaterial, Type, ParameterName, NodeX, NodeY);

    FCustomInput& DefaultInput = RaymarchNode->Inputs.Emplace_GetRef();
    DefaultInput.InputName = FName(ParameterName);
    DefaultInput.Input.Expression = Parameter;
  }
}

static void GenerateMaterialNodes(
    UCesiumVoxelMetadataComponent* pComponent,
    MaterialGenerationState& MaterialState,
    const MaterialResourceLibrary& ResourceLibrary) {
  UMaterial* pMaterial = pComponent->TargetMaterial;

  int32 NodeX = 0;
  int32 NodeY = 0;
  int32 DataSectionX = 0;
  int32 DataSectionY = 0;

  FMaterialExpressionCollection& SrcCollection =
      ResourceLibrary.pMaterialTemplate->GetExpressionCollection();
  TMap<const UMaterialExpression*, UMaterialExpression*> SrcToDestMap;
  MaterialState.AutoGeneratedNodes.Reserve(SrcCollection.Expressions.Num());

  UMaterialExpressionCustom* pRaymarchNode = nullptr;
  UMaterialExpressionMaterialFunctionCall* pBreakFloat4Node = nullptr;
  UMaterialExpression* pEyeAdaptationInverse = nullptr;

  for (const UMaterialExpression* pSrcExpression : SrcCollection.Expressions) {
    // Much of the code below is derived from
    // UMaterialExpression::CopyMaterialExpressions(), without some extra
    // modifications.
    UMaterialExpression* pNewExpression =
        Cast<UMaterialExpression>(StaticDuplicateObject(
            pSrcExpression,
            pMaterial,
            NAME_None,
            RF_Transactional));

    // Make sure we remove any references to materials or functions the nodes
    // came from.
    pNewExpression->Material = nullptr;
    pNewExpression->Function = nullptr;

    SrcToDestMap.Add(pSrcExpression, pNewExpression);

    // Add to list of autogenerated nodes.
    MaterialState.AutoGeneratedNodes.Add(pNewExpression);

    // There can be only one default mesh paint texture.
    UMaterialExpressionTextureBase* pTextureSample =
        Cast<UMaterialExpressionTextureBase>(pNewExpression);
    if (pTextureSample) {
      pTextureSample->IsDefaultMeshpaintTexture = false;
    }

    pNewExpression->UpdateParameterGuid(true, true);
    pNewExpression->UpdateMaterialExpressionGuid(true, true);
    // End of UMaterialExpression::CopyMaterialExpressions() body.

    auto* pCustomNode = Cast<UMaterialExpressionCustom>(pNewExpression);
    if (pCustomNode && pCustomNode->GetDescription() == RaymarchDescription) {
      pRaymarchNode = pCustomNode;
      continue;
    }

    auto* pMaterialFunctionNode =
        Cast<UMaterialExpressionMaterialFunctionCall>(pNewExpression);
    if (pMaterialFunctionNode) {
      pBreakFloat4Node = pMaterialFunctionNode;
      continue;
    }

    auto* pVectorParameterNode =
        Cast<UMaterialExpressionVectorParameter>(pNewExpression);
    if (pVectorParameterNode &&
        pVectorParameterNode->ParameterName.ToString() == "Tile Count") {
      DataSectionX = pVectorParameterNode->MaterialExpressionEditorX;
      DataSectionY = pVectorParameterNode->MaterialExpressionEditorY;
      continue;
    }

    // For some reason using UMaterialExpressionEyeAdaptationInverse results in
    // a linker error, even when its .h file is included.
    if (pNewExpression->GetName().Contains("EyeAdaptationInverse")) {
      pEyeAdaptationInverse = pNewExpression;
      continue;
    }
  }

  if (!pRaymarchNode) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Unable to generate material from M_CesiumVoxelMaterial template."))
    return;
  }

  // Fix up internal references.  Iterate over the inputs of the new
  // expressions, and for each input that refers to an expression that was
  // duplicated, point the reference to that new expression.  Otherwise, clear
  // the input.
  for (UMaterialExpression* pNewExpression : MaterialState.AutoGeneratedNodes) {
    for (FExpressionInputIterator It(pNewExpression); It; ++It) {
      UMaterialExpression* InputExpression = It.Input->Expression;
      if (InputExpression) {
        UMaterialExpression** NewInputExpression =
            SrcToDestMap.Find(InputExpression);
        if (NewInputExpression) {
          check(*NewInputExpression);
          It.Input->Expression = *NewInputExpression;
        } else {
          It.Input->Expression = nullptr;
        }
      }
    }
  }

  NodeX = DataSectionX;
  NodeY = DataSectionY;

  // Inspired by HLSLMaterialTranslator.cpp. Similar to MaterialTemplate.ush,
  // CesiumVoxelTemplate.usf contains "%s" formatters that will be replaced
  // with generated code.
  FLazyPrintf LazyPrintf(*ResourceLibrary.ShaderTemplate);
  CustomShaderBuilder Builder;

  const TArray<FCesiumPropertyAttributePropertyDescription>& Properties =
      pComponent->Description.Properties;

  pRaymarchNode->Inputs.Reserve(pRaymarchNode->Inputs.Num() + Properties.Num());
  for (const FCesiumPropertyAttributePropertyDescription& Property :
       Properties) {
    if (!isSupportedPropertyAttributeProperty(Property.PropertyDetails)) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "Property %s (%s) is not supported for voxels and will not be added to the generated material."),
          *Property.Name,
          *FCesiumMetadataValueType(
               Property.PropertyDetails.Type,
               Property.PropertyDetails.ComponentType,
               Property.PropertyDetails.bIsArray)
               .ToString());
      continue;
    }

    NodeY += Incr;

    FString PropertyName = createHlslSafeName(Property.Name);
    // Example: "temperature_DATA"
    FString PropertyDataName = PropertyName + MaterialPropertyDataSuffix;

    UMaterialExpressionTextureObjectParameter* PropertyData =
        NewObject<UMaterialExpressionTextureObjectParameter>(pMaterial);
    PropertyData->ParameterName = FName(PropertyName);
    PropertyData->MaterialExpressionEditorX = NodeX;
    PropertyData->MaterialExpressionEditorY = NodeY;
    // Set the initial value to default volume texture to avoid compilation
    // errors with the default 2D texture.
    PropertyData->Texture =
        Cast<UTexture>(ResourceLibrary.pDefaultVolumeTexture);
    MaterialState.AutoGeneratedNodes.Add(PropertyData);

    FCustomInput& PropertyInput = pRaymarchNode->Inputs.Emplace_GetRef();
    PropertyInput.InputName = FName(PropertyDataName);
    PropertyInput.Input.Expression = PropertyData;

    GenerateNodesForMetadataPropertyTransforms(
        pMaterial,
        MaterialState.AutoGeneratedNodes,
        Property.PropertyDetails,
        Property.EncodingDetails.Type,
        PropertyName,
        NodeX,
        NodeY,
        pRaymarchNode);

    Builder.AddShaderProperty(PropertyName, PropertyDataName, Property);
  }

  for (const FCesiumMetadataPropertyStatisticsDescription& propertyStatistics :
       pComponent->Description.Statistics) {
    for (const FCesiumMetadataPropertyStatisticValue& statistic :
         propertyStatistics.Values) {
      FCesiumMetadataValueType valueType =
          UCesiumMetadataValueBlueprintLibrary::GetValueType(statistic.Value);

      UMaterialExpressionParameter* pParameter = nullptr;
      switch (valueType.Type) {
      case ECesiumMetadataType::Scalar:
        pParameter = NewObject<UMaterialExpressionScalarParameter>(pMaterial);
        break;
      case ECesiumMetadataType::Vec2:
      case ECesiumMetadataType::Vec3:
      case ECesiumMetadataType::Vec4:
        pParameter = NewObject<UMaterialExpressionVectorParameter>(pMaterial);
        break;
      default:
        FString semanticName =
            StaticEnum<ECesiumMetadataStatisticSemantic>()
                ->GetNameStringByValue(int64(statistic.Semantic));
        FString enumName =
            StaticEnum<ECesiumMetadataType>()->GetNameStringByValue(
                int64(valueType.Type));
        UE_LOG(
            LogCesium,
            Warning,
            TEXT(
                "Skipping material node generation for %s %s in due to unsupported statistic type %s"),
            *propertyStatistics.Id,
            *semanticName,
            *enumName);
        continue;
      }

      NodeY += Incr;

      // Example: "temperature_MIN"
      FString statisticName = getNameForStatistic(
          FString(),
          propertyStatistics.Id,
          statistic.Semantic);

      pParameter->ParameterName = FName(statisticName);
      pParameter->MaterialExpressionEditorX = NodeX;
      pParameter->MaterialExpressionEditorY = NodeY;
      MaterialState.AutoGeneratedNodes.Add(pParameter);

      FCustomInput& PropertyInput = pRaymarchNode->Inputs.Emplace_GetRef();
      PropertyInput.InputName = FName(statisticName);
      PropertyInput.Input.Expression = pParameter;

      Builder.AddShaderStatistic(propertyStatistics.Id, statistic);
    }
  }

  LazyPrintf.PushParam(*Builder.DeclareShaderProperties);
  LazyPrintf.PushParam(*pComponent->AdditionalFunctions);
  LazyPrintf.PushParam(*pComponent->CustomShader);
  LazyPrintf.PushParam(*Builder.DeclareDataTextureVariables);
  LazyPrintf.PushParam(*Builder.SamplePropertiesFromTexture);
  LazyPrintf.PushParam(*Builder.SetDataTextures);
  LazyPrintf.PushParam(*Builder.SetStatistics);
  pRaymarchNode->Code = LazyPrintf.GetResultString();

  UMaterialEditorOnlyData* pEditorOnlyData = pMaterial->GetEditorOnlyData();
  CESIUM_ASSERT(pEditorOnlyData);

  if (!pEditorOnlyData->EmissiveColor.Expression) {
    pEditorOnlyData->EmissiveColor.Connect(0, pEyeAdaptationInverse);
  }

  if (!pEditorOnlyData->Opacity.Expression) {
    pEditorOnlyData->Opacity.Connect(3, pBreakFloat4Node);
  }
}

static void RemapUserConnections(
    UMaterial* pMaterial,
    TMap<FString, MaterialGenerationState::ParameterConnections>&
        connectionInputMap,
    TMap<FString, TArray<FExpressionInput*>>& connectionOutputMap) {

  VoxelMetadataClassification Classification = ClassifyNodes(pMaterial);

  UMaterialExpressionCustom* pRaymarchNode = Classification.RaymarchNode;
  if (pRaymarchNode && pRaymarchNode->Outputs.Num() > 0) {
    FExpressionOutput& Output = pRaymarchNode->Outputs[0];
    FString Key =
        pRaymarchNode->GetDescription() + Output.OutputName.ToString();

    TArray<FExpressionInput*>* pConnections = connectionOutputMap.Find(Key);
    if (pConnections) {
      for (FExpressionInput* pConnection : *pConnections) {
        pConnection->Connect(0, pRaymarchNode);
      }
    }
  }

  if (Classification.BreakFloat4Node) {
    int32 OutputIndex = 0;
    for (const FExpressionOutput& Output :
         Classification.BreakFloat4Node->Outputs) {
      FString Key = Classification.BreakFloat4Node->GetDescription() +
                    Output.OutputName.ToString();

      TArray<FExpressionInput*>* pConnections = connectionOutputMap.Find(Key);
      if (pConnections) {
        for (FExpressionInput* pConnection : *pConnections) {
          pConnection->Connect(OutputIndex, Classification.BreakFloat4Node);
        }
      }

      ++OutputIndex;
    }
  }
}

void UCesiumVoxelMetadataComponent::GenerateMaterial() {
  ACesium3DTileset* pTileset = Cast<ACesium3DTileset>(this->GetOwner());
  if (!pTileset) {
    return;
  }

  MaterialResourceLibrary ResourceLibrary;
  ResourceLibrary.pDefaultVolumeTexture = this->_pDefaultVolumeTexture;
  if (!ResourceLibrary.isValid()) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Can't find the material or shader templates necessary to generate voxel material. Aborting."));
    return;
  }

  if (this->TargetMaterial && this->TargetMaterial->GetPackage()->IsDirty()) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Can't regenerate a material layer that has unsaved changes. Please save your changes and try again."));
    return;
  }

  const FString MaterialName =
      "M_" + pTileset->GetFName().ToString() + "_VoxelMetadata";
  const FString PackageBaseName = "/Game/";
  const FString PackageName = PackageBaseName + MaterialName;

  bool Overwriting = false;
  if (this->TargetMaterial) {
    // Overwriting an existing material layer.
    Overwriting = true;
    GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()
        ->CloseAllEditorsForAsset(this->TargetMaterial);
  } else {
    this->TargetMaterial = CreateMaterial(PackageName, MaterialName);
  }

  this->TargetMaterial->PreEditChange(NULL);

  MaterialGenerationState MaterialState;

  ClearAutoGeneratedNodes(
      this->TargetMaterial,
      MaterialState.UserConnectionInputMap,
      MaterialState.UserConnectionOutputMap);
  GenerateMaterialNodes(this, MaterialState, ResourceLibrary);
  MoveNodesToMaterial(MaterialState, this->TargetMaterial);

  RemapUserConnections(
      this->TargetMaterial,
      MaterialState.UserConnectionInputMap,
      MaterialState.UserConnectionOutputMap);

  this->TargetMaterial->BlendMode =
      TEnumAsByte<EBlendMode>(EBlendMode::BLEND_Translucent);
  this->TargetMaterial->SetShadingModel(EMaterialShadingModel::MSM_Unlit);

  // Let the material update itself if necessary
  this->TargetMaterial->PostEditChange();

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
      AssetsToHighlight.Add(this->TargetMaterial);
      pContentBrowserModule->Get().SyncBrowserToAssets(AssetsToHighlight);
    }
  }

  // Open the updated material in editor.
  if (GEditor) {
    UAssetEditorSubsystem* pAssetEditor =
        GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
    if (pAssetEditor) {
      GEngine->EndTransaction();
      pAssetEditor->OpenEditorForAsset(this->TargetMaterial);
      IMaterialEditor* pMaterialEditor = static_cast<IMaterialEditor*>(
          pAssetEditor->FindEditorForAsset(this->TargetMaterial, true));
      if (pMaterialEditor) {
        pMaterialEditor->UpdateMaterialAfterGraphChange();
      }
    }
  }
}

#endif // WITH_EDITOR
