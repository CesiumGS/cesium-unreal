// Copyright 2020-2024 CesiumGS, Inc. and Contributors

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
#include "UnrealMetadataConversions.h"

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
#include "Materials/MaterialExpressionReroute.h"
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

#include <Cesium3DTiles/Class.h>

using namespace EncodedFeaturesMetadata;
using namespace GenerateMaterialUtility;

static const FString RaymarchDescription = "Voxel Raymarch";

UCesiumVoxelMetadataComponent::UCesiumVoxelMetadataComponent()
    : UActorComponent() {
  // Structure to hold one-time initialization
  struct FConstructorStatics {
    ConstructorHelpers::FObjectFinder<UVolumeTexture> DefaultVolumeTexture;
    FConstructorStatics()
        : DefaultVolumeTexture(
              TEXT("/Engine/EngineResources/DefaultVolumeTexture")) {}
  };
  static FConstructorStatics ConstructorStatics;
  DefaultVolumeTexture = ConstructorStatics.DefaultVolumeTexture.Object;

#if WITH_EDITOR
  this->UpdateShaderPreview();
#endif
}

#if WITH_EDITOR
void UCesiumVoxelMetadataComponent::PostLoad() {
  Super::PostLoad();
  this->UpdateShaderPreview();
}

void UCesiumVoxelMetadataComponent::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) {
  Super::PostEditChangeProperty(PropertyChangedEvent);

  if (!PropertyChangedEvent.Property) {
    return;
  }

  FName PropName = PropertyChangedEvent.Property->GetFName();
  FString PropNameAsString = PropertyChangedEvent.Property->GetName();

  if (!PropertyChangedEvent.Property) {
    return;
  }

  if (PropName == GET_MEMBER_NAME_CHECKED(
                      UCesiumVoxelMetadataComponent,
                      CustomShader) ||
      PropName == GET_MEMBER_NAME_CHECKED(
                      UCesiumVoxelMetadataComponent,
                      AdditionalFunctions) ||
      PropName ==
          GET_MEMBER_NAME_CHECKED(UCesiumVoxelMetadataComponent, Description)) {
    this->UpdateShaderPreview();
  }
}

void UCesiumVoxelMetadataComponent::PostEditChangeChainProperty(
    FPropertyChangedChainEvent& PropertyChangedChainEvent) {
  Super::PostEditChangeChainProperty(PropertyChangedChainEvent);
  if (!PropertyChangedChainEvent.Property ||
      PropertyChangedChainEvent.PropertyChain.IsEmpty()) {
    return;
  }
  this->UpdateShaderPreview();
}

namespace {
FCesiumMetadataValueType
GetValueTypeFromClassProperty(const Cesium3DTiles::ClassProperty& Property) {
  FCesiumMetadataValueType ValueType;
  ValueType.Type = (ECesiumMetadataType)CesiumGltf::convertStringToPropertyType(
      Property.type);
  ValueType.ComponentType = (ECesiumMetadataComponentType)
      CesiumGltf::convertStringToPropertyComponentType(
          Property.componentType.value_or(""));
  ValueType.bIsArray = Property.array;
  return ValueType;
}

void AutoFillVoxelClassDescription(
    FCesiumVoxelClassDescription& Description,
    const std::string& VoxelClassID,
    const Cesium3DTiles::Class& VoxelClass) {
  Description.ID = VoxelClassID.c_str();

  for (const auto& propertyIt : VoxelClass.properties) {
    auto pExistingProperty = Description.Properties.FindByPredicate(
        [&propertyName = propertyIt.first](
            const FCesiumPropertyAttributePropertyDescription&
                existingProperty) {
          return existingProperty.Name == propertyName.c_str();
        });

    if (pExistingProperty) {
      // We have already accounted for this property.
      continue;
    }

    FCesiumPropertyAttributePropertyDescription& property =
        Description.Properties.Emplace_GetRef();
    property.Name = propertyIt.first.c_str();

    property.PropertyDetails.SetValueType(
        GetValueTypeFromClassProperty(propertyIt.second));
    property.PropertyDetails.ArraySize = propertyIt.second.count.value_or(0);
    property.PropertyDetails.bIsNormalized = propertyIt.second.normalized;

    // These values are not actually validated until the material is generated.
    property.PropertyDetails.bHasOffset = propertyIt.second.offset.has_value();
    property.PropertyDetails.bHasScale = propertyIt.second.scale.has_value();
    property.PropertyDetails.bHasNoDataValue =
        propertyIt.second.noData.has_value();
    property.PropertyDetails.bHasDefaultValue =
        propertyIt.second.defaultProperty.has_value();

    property.EncodingDetails = CesiumMetadataPropertyDetailsToEncodingDetails(
        property.PropertyDetails);
  }
}
} // namespace

void UCesiumVoxelMetadataComponent::AutoFill() {
  const ACesium3DTileset* pOwner = this->GetOwner<ACesium3DTileset>();
  const Cesium3DTilesSelection::Tileset* pTileset =
      pOwner ? pOwner->GetTileset() : nullptr;
  if (!pTileset) {
    return;
  }

  const Cesium3DTiles::ExtensionContent3dTilesContentVoxels* pVoxelExtension =
      pTileset->getVoxelContentExtension();
  if (!pVoxelExtension) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "Tileset %s does not contain voxel content, so CesiumVoxelMetadataComponent will have no effect."),
        *pOwner->GetName());
    return;
  }

  // TODO turn into helper? function
  const Cesium3DTilesSelection::TilesetMetadata* pMetadata =
      pTileset->getMetadata();
  if (!pMetadata || !pMetadata->schema) {
    return;
  }

  const std::string& voxelClassId = pVoxelExtension->classProperty;
  if (pMetadata->schema->classes.find(voxelClassId) ==
      pMetadata->schema->classes.end()) {
    return;
  }

  Super::PreEditChange(NULL);

  AutoFillVoxelClassDescription(
      this->Description,
      voxelClassId,
      pMetadata->schema->classes.at(voxelClassId));

  Super::PostEditChange();

  UpdateShaderPreview();
}

namespace {
struct VoxelMetadataClassification : public MaterialNodeClassification {
  UMaterialExpressionCustom* RaymarchNode = nullptr;
  UMaterialExpressionMaterialFunctionCall* BreakFloat4Node = nullptr;
};

struct MaterialResourceLibrary {
  FString HlslShaderTemplate;
  UMaterialFunctionMaterialLayer* MaterialLayerTemplate;
  UVolumeTexture* DefaultVolumeTexture;

  MaterialResourceLibrary() {
    static FString ContentDir = IPluginManager::Get()
                                    .FindPlugin(TEXT("CesiumForUnreal"))
                                    ->GetContentDir();
    FFileHelper::LoadFileToString(
        HlslShaderTemplate,
        *(ContentDir / "Materials/CesiumVoxelTemplate.hlsl"));

    MaterialLayerTemplate = LoadObjFromPath<UMaterialFunctionMaterialLayer>(
        "/CesiumForUnreal/Materials/Layers/ML_CesiumVoxel");
  }

  bool isValid() const {
    return !HlslShaderTemplate.IsEmpty() && MaterialLayerTemplate &&
           DefaultVolumeTexture;
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
      // clang-format off
      DeclareShaderProperties += "\t" + encodedHlslType + " " + rawPropertyName +
                                 ";\n\t" +
                                 normalizedHlslType + " " + PropertyName + ";";
      // clang-format on
    } else {
      // e.g., "float temperature;"
      DeclareShaderProperties +=
          "\t" + encodedHlslType + " " + PropertyName + ";";
    }

    if (Property.PropertyDetails.bHasNoDataValue) {
      // Expose "no data" value to the shader so the user can act on it.
      // "No data" values are always given in the raw value type.
      FString NoDataName = PropertyName + MaterialPropertyNoDataSuffix;
      DeclareShaderProperties +=
          "\n\t" + encodedHlslType + " " + NoDataName + ";";
    }

    if (Property.PropertyDetails.bHasDefaultValue) {
      // Expose default value to the shader so the user can act on it.
      FString DefaultValueName =
          PropertyName + MaterialPropertyDefaultValueSuffix;
      DeclareShaderProperties +=
          "\n\t" + isNormalizedProperty ? normalizedHlslType : encodedHlslType;
      DeclareShaderProperties += " " + DefaultValueName + ";";
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
    DeclareDataTextureVariables += "Texture3D " + PropertyName + ";";

    if (!SetDataTextures.IsEmpty()) {
      SetDataTextures += "\n";
    }
    // e.g., "DataTextures.temperature = temperature_DATA;"
    SetDataTextures +=
        "DataTextures." + PropertyName + " = " + TextureParameterName + ";";
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
      SamplePropertiesFromTexture += "Properties." + rawPropertyName + " = " +
                                     PropertyName + ".Load(int4(Coords, 0))" +
                                     swizzle + ";";
      // Normalization can be hardcoded because only normalized uint8s are
      // supported.
      SamplePropertiesFromTexture += "\n\t\tProperties." + PropertyName +
                                     " = (Properties." + rawPropertyName +
                                     " / 255.0)";
    } else {
      SamplePropertiesFromTexture += "Properties." + PropertyName + " = " +
                                     PropertyName + ".Load(int4(Coords, 0))" +
                                     swizzle;
    }

    if (Property.PropertyDetails.bHasScale) {
      FString ScaleName = PropertyName + MaterialPropertyScaleSuffix;
      // Declare the value transforms underneath the corresponding data texture
      // variable. e.g., float myProperty_SCALE;
      DeclareDataTextureVariables +=
          "\n\t" + isNormalizedProperty ? normalizedHlslType : encodedHlslType;
      DeclareDataTextureVariables += " " + ScaleName + ";";
      SetDataTextures +=
          "\nDataTextures." + ScaleName + " = " + ScaleName + ";";

      // e.g., " * myProperty_SCALE"
      SamplePropertiesFromTexture += " * " + ScaleName;
    }

    if (Property.PropertyDetails.bHasOffset) {
      FString OffsetName = PropertyName + MaterialPropertyOffsetSuffix;
      DeclareDataTextureVariables +=
          "\n\t" + isNormalizedProperty ? normalizedHlslType : encodedHlslType;
      DeclareDataTextureVariables += " " + OffsetName + ";";
      SetDataTextures +=
          "\nDataTextures." + OffsetName + " = " + OffsetName + ";";

      // e.g., " + myProperty_OFFSET"
      SamplePropertiesFromTexture += " + " + OffsetName;
    }

    SamplePropertiesFromTexture += ";";

    if (Property.PropertyDetails.bHasNoDataValue) {
      FString NoDataName = PropertyName + MaterialPropertyNoDataSuffix;
      DeclareDataTextureVariables +=
          "\n\t" + encodedHlslType + " " + NoDataName + ";";
      SetDataTextures +=
          "\nDataTextures." + NoDataName + " = " + NoDataName + ";";

      SamplePropertiesFromTexture +=
          "\n\tProperties." + NoDataName + " = " + NoDataName + ";";
    }

    if (Property.PropertyDetails.bHasDefaultValue) {
      FString DefaultValueName =
          PropertyName + MaterialPropertyDefaultValueSuffix;
      DeclareDataTextureVariables +=
          "\n\t" + isNormalizedProperty ? normalizedHlslType : encodedHlslType;
      DeclareDataTextureVariables += " " + DefaultValueName + ";";
      SetDataTextures +=
          "\nDataTextures." + DefaultValueName + " = " + DefaultValueName + ";";

      SamplePropertiesFromTexture +=
          "\n\tProperties." + DefaultValueName + " = " + DefaultValueName + ";";
    }
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

/*static*/ const FString UCesiumVoxelMetadataComponent::ShaderPreviewTemplate =
    "struct CustomShaderProperties {\n"
    "%s"
    "\n}\n\n"
    "struct CustomShader {\n"
    "%s\n\n"
    "\tfloat4 Shade(CustomShaderProperties Properties) {\n"
    "%s\n"
    "\t}\n}";
#endif

void UCesiumVoxelMetadataComponent::UpdateShaderPreview() {
  // Inspired by HLSLMaterialTranslator.cpp. Similar to MaterialTemplate.ush,
  // CesiumVoxelTemplate.ush contains "%s" formatters that should be replaced
  // with generated code.
  FLazyPrintf LazyPrintf(*ShaderPreviewTemplate);
  CustomShaderBuilder Builder;

  const TArray<FCesiumPropertyAttributePropertyDescription>& Properties =
      this->Description.Properties;
  for (const FCesiumPropertyAttributePropertyDescription& Property :
       Properties) {
    if (!isSupportedPropertyAttributeProperty(Property.PropertyDetails)) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "Property %s of type %s, component type %s is not supported for voxels and will not be added to the generated material."),
          *Property.Name,
          *MetadataTypeToString(Property.PropertyDetails.Type),
          *MetadataComponentTypeToString(
              Property.PropertyDetails.ComponentType));
      continue;
    }

    FString PropertyName = createHlslSafeName(Property.Name);
    Builder.AddPropertyDeclaration(PropertyName, Property);
  }

  LazyPrintf.PushParam(*Builder.DeclareShaderProperties);
  LazyPrintf.PushParam(*this->AdditionalFunctions);
  LazyPrintf.PushParam(*this->CustomShader);

  this->CustomShaderPreview = LazyPrintf.GetResultString();
}

static VoxelMetadataClassification
ClassifyNodes(UMaterialFunctionMaterialLayer* Layer) {
  VoxelMetadataClassification Classification;
  for (const TObjectPtr<UMaterialExpression>& pNode :
       Layer->GetExpressionCollection().Expressions) {
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
          pFunctionCall && pFunctionCall->MaterialFunction
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
    UMaterialFunctionMaterialLayer* Layer,
    TMap<FString, TMap<FString, const FExpressionInput*>>& ConnectionInputRemap,
    TMap<FString, TArray<FExpressionInput*>>& ConnectionOutputRemap) {
  VoxelMetadataClassification Classification = ClassifyNodes(Layer);
  // Determine which user-added connections to remap when regenerating the
  // voxel raymarch node.
  UMaterialExpressionCustom* pRaymarchNode = Classification.RaymarchNode;
  if (pRaymarchNode && pRaymarchNode->Outputs.Num() > 0) {
    FExpressionOutput& Output = pRaymarchNode->Outputs[0];
    FString Key =
        pRaymarchNode->GetDescription() + Output.OutputName.ToString();

    // Look for user-made connections to this property.
    TArray<FExpressionInput*> Connections;
    for (UMaterialExpression* UserNode : Classification.UserAddedNodes) {
      for (FExpressionInput* Input : UserNode->GetInputsView()) {
        if (Input->Expression == pRaymarchNode && Input->OutputIndex == 0) {
          Connections.Add(Input);
          Input->Expression = nullptr;
        }
      }
    }

    ConnectionOutputRemap.Emplace(MoveTemp(Key), MoveTemp(Connections));
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
      for (UMaterialExpression* UserNode : Classification.UserAddedNodes) {
        for (FExpressionInput* Input : UserNode->GetInputsView()) {
          if (Input->Expression == pBreakNode &&
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

  // Remove auto-generated nodes.
  for (UMaterialExpression* AutoGeneratedNode :
       Classification.AutoGeneratedNodes) {
    Layer->GetExpressionCollection().RemoveExpression(AutoGeneratedNode);
  }
}

/**
 * @brief Generates the nodes necessary to apply property transforms to a
 * metadata property.
 */
static void GenerateNodesForMetadataPropertyTransforms(
    UMaterialFunctionMaterialLayer* Layer,
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
        GenerateParameterNode(Layer, Type, ParameterName, NodeX, NodeY);
    AutoGeneratedNodes.Add(Parameter);

    FCustomInput& ScaleInput = RaymarchNode->Inputs.Emplace_GetRef();
    ScaleInput.InputName = FName(ParameterName);
    ScaleInput.Input.Expression = Parameter;
  }

  if (PropertyDetails.bHasOffset) {
    NodeY += Incr;
    FString ParameterName = PropertyName + MaterialPropertyOffsetSuffix;
    UMaterialExpressionParameter* Parameter =
        GenerateParameterNode(Layer, Type, ParameterName, NodeX, NodeY);
    AutoGeneratedNodes.Add(Parameter);

    FCustomInput& OffsetInput = RaymarchNode->Inputs.Emplace_GetRef();
    OffsetInput.InputName = FName(ParameterName);
    OffsetInput.Input.Expression = Parameter;
  }

  FString swizzle = GetSwizzleForEncodedType(Type);

  if (PropertyDetails.bHasNoDataValue) {
    NodeY += Incr;
    FString ParameterName = PropertyName + MaterialPropertyNoDataSuffix;
    UMaterialExpressionParameter* Parameter =
        GenerateParameterNode(Layer, Type, ParameterName, NodeX, NodeY);
    AutoGeneratedNodes.Add(Parameter);

    FCustomInput& NoDataInput = RaymarchNode->Inputs.Emplace_GetRef();
    NoDataInput.InputName = FName(ParameterName);
    NoDataInput.Input.Expression = Parameter;
  }

  if (PropertyDetails.bHasDefaultValue) {
    NodeY += Incr;
    FString ParameterName = PropertyName + MaterialPropertyDefaultValueSuffix;
    UMaterialExpressionParameter* Parameter =
        GenerateParameterNode(Layer, Type, ParameterName, NodeX, NodeY);

    FCustomInput& DefaultInput = RaymarchNode->Inputs.Emplace_GetRef();
    DefaultInput.InputName = FName(ParameterName);
    DefaultInput.Input.Expression = Parameter;
  }
}

static void GenerateMaterialNodes(
    UCesiumVoxelMetadataComponent* Component,
    MaterialGenerationState& MaterialState,
    const MaterialResourceLibrary& ResourceLibrary) {
  UMaterialFunctionMaterialLayer* pTargetLayer = Component->TargetMaterialLayer;

  int32 NodeX = 0;
  int32 NodeY = 0;
  int32 DataSectionX = 0;
  int32 DataSectionY = 0;

  FMaterialExpressionCollection& SrcCollection =
      ResourceLibrary.MaterialLayerTemplate->GetExpressionCollection();
  TMap<const UMaterialExpression*, UMaterialExpression*> SrcToDestMap;
  MaterialState.AutoGeneratedNodes.Reserve(SrcCollection.Expressions.Num());

  UMaterialExpressionCustom* RaymarchNode = nullptr;
  UMaterialExpressionMaterialFunctionCall* BreakFloat4Node = nullptr;

  for (const UMaterialExpression* SrcExpression : SrcCollection.Expressions) {
    // Ignore the standard input / output nodes; these do not need duplication.
    const auto* InputMaterial =
        Cast<UMaterialExpressionFunctionInput>(SrcExpression);
    const auto* SetAttributes =
        Cast<UMaterialExpressionSetMaterialAttributes>(SrcExpression);
    const auto* OutputMaterial =
        Cast<UMaterialExpressionFunctionOutput>(SrcExpression);
    if (InputMaterial || SetAttributes || OutputMaterial) {
      continue;
    }

    // Much of the code below is derived from
    // UMaterialExpression::CopyMaterialExpressions().
    UMaterialExpression* NewExpression =
        Cast<UMaterialExpression>(StaticDuplicateObject(
            SrcExpression,
            pTargetLayer,
            NAME_None,
            RF_Transactional));

    // Make sure we remove any references to materials or functions the nodes
    // came from.
    NewExpression->Material = nullptr;
    NewExpression->Function = nullptr;

    SrcToDestMap.Add(SrcExpression, NewExpression);

    // Add to list of autogenerated nodes.
    MaterialState.AutoGeneratedNodes.Add(NewExpression);

    // There can be only one default mesh paint texture.
    UMaterialExpressionTextureBase* TextureSample =
        Cast<UMaterialExpressionTextureBase>(NewExpression);
    if (TextureSample) {
      TextureSample->IsDefaultMeshpaintTexture = false;
    }

    NewExpression->UpdateParameterGuid(true, true);
    NewExpression->UpdateMaterialExpressionGuid(true, true);

    auto* CustomNode = Cast<UMaterialExpressionCustom>(NewExpression);
    if (CustomNode && CustomNode->GetDescription() == RaymarchDescription) {
      RaymarchNode = CustomNode;
      continue;
    }

    auto* MaterialFunctionNode =
        Cast<UMaterialExpressionMaterialFunctionCall>(NewExpression);
    if (MaterialFunctionNode) {
      BreakFloat4Node = MaterialFunctionNode;
      continue;
    }

    auto* VectorParameterNode =
        Cast<UMaterialExpressionVectorParameter>(NewExpression);
    if (VectorParameterNode &&
        VectorParameterNode->ParameterName.ToString() == "Tile Count") {
      DataSectionX = VectorParameterNode->MaterialExpressionEditorX;
      DataSectionY = VectorParameterNode->MaterialExpressionEditorY;
    }
  }

  if (!RaymarchNode) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT("Unable to generate material from ML_CesiumVoxels template."))
    return;
  }

  // Fix up internal references.  Iterate over the inputs of the new
  // expressions, and for each input that refers to an expression that was
  // duplicated, point the reference to that new expression.  Otherwise, clear
  // the input.
  for (UMaterialExpression* NewExpression : MaterialState.AutoGeneratedNodes) {
    const TArrayView<FExpressionInput*>& ExpressionInputs =
        NewExpression->GetInputsView();
    for (int32 ExpressionInputIndex = 0;
         ExpressionInputIndex < ExpressionInputs.Num();
         ++ExpressionInputIndex) {
      FExpressionInput* Input = ExpressionInputs[ExpressionInputIndex];
      UMaterialExpression* InputExpression = Input->Expression;
      if (InputExpression) {
        UMaterialExpression** NewInputExpression =
            SrcToDestMap.Find(InputExpression);
        if (NewInputExpression) {
          check(*NewInputExpression);
          Input->Expression = *NewInputExpression;
        } else {
          Input->Expression = nullptr;
        }
      }
    }
  }

  // Save this to offset some nodes later.
  int32 SetMaterialAttributesOffset =
      BreakFloat4Node->MaterialExpressionEditorX;
  NodeX = DataSectionX;
  NodeY = DataSectionY;

  // Inspired by HLSLMaterialTranslator.cpp. Similar to MaterialTemplate.ush,
  // CesiumVoxelTemplate.hlsl contains "%s" formatters that will be replaced
  // with generated code.
  FLazyPrintf LazyPrintf(*ResourceLibrary.HlslShaderTemplate);
  CustomShaderBuilder Builder;

  const TArray<FCesiumPropertyAttributePropertyDescription>& Properties =
      Component->Description.Properties;
  RaymarchNode->Inputs.Reserve(RaymarchNode->Inputs.Num() + Properties.Num());
  for (const FCesiumPropertyAttributePropertyDescription& Property :
       Properties) {
    if (!isSupportedPropertyAttributeProperty(Property.PropertyDetails)) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "Property %s of type %s, component type %s is not supported for voxels and will not be added to the generated material."),
          *Property.Name,
          *MetadataTypeToString(Property.PropertyDetails.Type),
          *MetadataComponentTypeToString(
              Property.PropertyDetails.ComponentType));
      continue;
    }

    NodeY += Incr;

    FString PropertyName = createHlslSafeName(Property.Name);
    // Example: "temperature_DATA"
    FString PropertyDataName = PropertyName + MaterialPropertyDataSuffix;

    UMaterialExpressionTextureObjectParameter* PropertyData =
        NewObject<UMaterialExpressionTextureObjectParameter>(pTargetLayer);
    PropertyData->ParameterName = FName(PropertyName);
    PropertyData->MaterialExpressionEditorX = NodeX;
    PropertyData->MaterialExpressionEditorY = NodeY;
    // Set the initial value to default volume texture to avoid compilation
    // errors with the default 2D texture.
    PropertyData->Texture = ResourceLibrary.DefaultVolumeTexture;
    MaterialState.AutoGeneratedNodes.Add(PropertyData);

    FCustomInput& PropertyInput = RaymarchNode->Inputs.Emplace_GetRef();
    PropertyInput.InputName = FName(PropertyDataName);
    PropertyInput.Input.Expression = PropertyData;

    GenerateNodesForMetadataPropertyTransforms(
        pTargetLayer,
        MaterialState.AutoGeneratedNodes,
        Property.PropertyDetails,
        Property.EncodingDetails.Type,
        PropertyName,
        NodeX,
        NodeY,
        RaymarchNode);

    Builder.AddShaderProperty(PropertyName, PropertyDataName, Property);
  }

  LazyPrintf.PushParam(*Builder.DeclareShaderProperties);
  LazyPrintf.PushParam(*Component->AdditionalFunctions);
  LazyPrintf.PushParam(*Component->CustomShader);
  LazyPrintf.PushParam(*Builder.DeclareDataTextureVariables);
  LazyPrintf.PushParam(*Builder.SamplePropertiesFromTexture);
  LazyPrintf.PushParam(*Builder.SetDataTextures);

  RaymarchNode->Code = LazyPrintf.GetResultString();

  UMaterialExpressionFunctionInput* InputMaterial = nullptr;
  for (const TObjectPtr<UMaterialExpression>& ExistingNode :
       pTargetLayer->GetExpressionCollection().Expressions) {
    UMaterialExpressionFunctionInput* ExistingInputMaterial =
        Cast<UMaterialExpressionFunctionInput>(ExistingNode);
    if (ExistingInputMaterial) {
      InputMaterial = ExistingInputMaterial;
      break;
    }
  }

  NodeX = 0;
  NodeY = 0;

  if (!InputMaterial) {
    InputMaterial = NewObject<UMaterialExpressionFunctionInput>(pTargetLayer);
    InputMaterial->InputType =
        EFunctionInputType::FunctionInput_MaterialAttributes;
    InputMaterial->bUsePreviewValueAsDefault = true;
    InputMaterial->MaterialExpressionEditorX = NodeX;
    InputMaterial->MaterialExpressionEditorY = NodeY;
    MaterialState.OneTimeGeneratedNodes.Add(InputMaterial);
  }

  NodeX += SetMaterialAttributesOffset + Incr;

  UMaterialExpressionSetMaterialAttributes* SetMaterialAttributes = nullptr;
  for (const TObjectPtr<UMaterialExpression>& ExistingNode :
       pTargetLayer->GetExpressionCollection().Expressions) {
    UMaterialExpressionSetMaterialAttributes* ExistingSetAttributes =
        Cast<UMaterialExpressionSetMaterialAttributes>(ExistingNode);
    if (ExistingSetAttributes) {
      SetMaterialAttributes = ExistingSetAttributes;
      break;
    }
  }

  if (!SetMaterialAttributes) {
    SetMaterialAttributes =
        NewObject<UMaterialExpressionSetMaterialAttributes>(pTargetLayer);
    SetMaterialAttributes->MaterialExpressionEditorX = NodeX;
    SetMaterialAttributes->MaterialExpressionEditorY = NodeY;
    MaterialState.OneTimeGeneratedNodes.Add(SetMaterialAttributes);
  }

  if (SetMaterialAttributes->Inputs.Num() <= 1) {
    SetMaterialAttributes->Inputs.Reset(3);
    SetMaterialAttributes->AttributeSetTypes.Reset(2);

    SetMaterialAttributes->Inputs.EmplaceAt(0, FExpressionInput());
    SetMaterialAttributes->Inputs[0].Expression = InputMaterial;

    SetMaterialAttributes->Inputs.EmplaceAt(1, FExpressionInput());
    SetMaterialAttributes->Inputs[1].Connect(0, RaymarchNode);
    SetMaterialAttributes->Inputs[1].InputName = "Base Color";

    SetMaterialAttributes->Inputs.EmplaceAt(2, FExpressionInput());
    SetMaterialAttributes->Inputs[2].Connect(3, BreakFloat4Node);
    SetMaterialAttributes->Inputs[2].InputName = "Opacity";

    // SetMaterialAttributes needs to manage an internal list of which
    // attributes were selected.
    const TArray<FGuid>& OrderedVisibleAttributes =
        FMaterialAttributeDefinitionMap::GetOrderedVisibleAttributeList();
    for (const FGuid& AttributeID : OrderedVisibleAttributes) {
      const FString& name =
          FMaterialAttributeDefinitionMap::GetAttributeName(AttributeID);
      if (name == "BaseColor") {
        SetMaterialAttributes->AttributeSetTypes.EmplaceAt(0, AttributeID);
      } else if (name == "Opacity") {
        SetMaterialAttributes->AttributeSetTypes.EmplaceAt(1, AttributeID);
      }
    }
  }

  NodeX += 2 * Incr;

  UMaterialExpressionFunctionOutput* OutputMaterial = nullptr;
  for (const TObjectPtr<UMaterialExpression>& ExistingNode :
       pTargetLayer->GetExpressionCollection().Expressions) {
    UMaterialExpressionFunctionOutput* ExistingOutputMaterial =
        Cast<UMaterialExpressionFunctionOutput>(ExistingNode);
    if (ExistingOutputMaterial) {
      OutputMaterial = ExistingOutputMaterial;
      break;
    }
  }

  if (!OutputMaterial) {
    OutputMaterial = NewObject<UMaterialExpressionFunctionOutput>(pTargetLayer);
    OutputMaterial->A = FMaterialAttributesInput();
    OutputMaterial->A.Expression = SetMaterialAttributes;
    OutputMaterial->MaterialExpressionEditorX = NodeX;
    OutputMaterial->MaterialExpressionEditorY = NodeY;
    MaterialState.OneTimeGeneratedNodes.Add(OutputMaterial);
  }
}

static void RemapUserConnections(
    UMaterialFunctionMaterialLayer* Layer,
    TMap<FString, TMap<FString, const FExpressionInput*>>& ConnectionInputRemap,
    TMap<FString, TArray<FExpressionInput*>>& ConnectionOutputRemap) {

  VoxelMetadataClassification Classification = ClassifyNodes(Layer);

  UMaterialExpressionCustom* pRaymarchNode = Classification.RaymarchNode;
  if (pRaymarchNode && pRaymarchNode->Outputs.Num() > 0) {
    FExpressionOutput& Output = pRaymarchNode->Outputs[0];
    FString Key =
        pRaymarchNode->GetDescription() + Output.OutputName.ToString();

    TArray<FExpressionInput*>* pConnections = ConnectionOutputRemap.Find(Key);
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

      TArray<FExpressionInput*>* pConnections = ConnectionOutputRemap.Find(Key);
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
  ResourceLibrary.DefaultVolumeTexture = this->DefaultVolumeTexture;
  if (!ResourceLibrary.isValid()) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Can't find the material or shader templates necessary to generate voxel material. Aborting."));
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

  const FString MaterialName =
      "ML_" + pTileset->GetFName().ToString() + "_VoxelMetadata";
  const FString PackageBaseName = "/Game/";
  const FString PackageName = PackageBaseName + MaterialName;

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
      MaterialState.ConnectionOutputRemap);
  GenerateMaterialNodes(this, MaterialState, ResourceLibrary);
  MoveNodesToMaterialLayer(MaterialState, this->TargetMaterialLayer);

  RemapUserConnections(
      this->TargetMaterialLayer,
      MaterialState.ConnectionInputRemap,
      MaterialState.ConnectionOutputRemap);

  this->TargetMaterialLayer->PreviewBlendMode =
      TEnumAsByte<EBlendMode>(EBlendMode::BLEND_Translucent);

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
