// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumFeaturesMetadataViewer.h"

#include "Cesium3DTileset.h"
#include "CesiumCommon.h"
#include "CesiumEditor.h"
#include "CesiumFeaturesMetadataComponent.h"
#include "CesiumMetadataEncodingDetails.h"
#include "CesiumModelMetadata.h"
#include "CesiumPrimitiveFeatures.h"
#include "CesiumPrimitiveMetadata.h"
#include "CesiumRuntimeSettings.h"
#include "EditorStyleSet.h"
#include "LevelEditor.h"
#include "PropertyCustomizationHelpers.h"
#include "ScopedTransaction.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SHeader.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"

THIRD_PARTY_INCLUDES_START
#include <Cesium3DTiles/Statistics.h>
#include <Cesium3DTilesSelection/Tileset.h>
#include <Cesium3DTilesSelection/TilesetMetadata.h>
#include <CesiumUtility/Uri.h>
THIRD_PARTY_INCLUDES_END

/*static*/ TSharedPtr<CesiumFeaturesMetadataViewer>
    CesiumFeaturesMetadataViewer::_pExistingWindow = nullptr;

/*static*/ void
CesiumFeaturesMetadataViewer::Open(TWeakObjectPtr<ACesium3DTileset> pTileset) {
  if (_pExistingWindow.IsValid()) {
    _pExistingWindow->_pTileset = pTileset;
    _pExistingWindow->BringToFront();
    _pExistingWindow->Sync();
  } else {
    // Open a new panel
    TSharedRef<CesiumFeaturesMetadataViewer> viewer =
        SNew(CesiumFeaturesMetadataViewer).Tileset(pTileset);

    _pExistingWindow = viewer;

    _pExistingWindow->GetOnWindowClosedEvent().AddLambda(
        [&pExistingWindow = CesiumFeaturesMetadataViewer::_pExistingWindow](
            const TSharedRef<SWindow>& pWindow) { pExistingWindow = nullptr; });
    FSlateApplication::Get().AddWindow(viewer);
  }

  if (pTileset.IsValid()) {
    _pExistingWindow->_pFeaturesMetadataComponent =
        pTileset->GetComponentByClass<UCesiumFeaturesMetadataComponent>();
  }
}

void CesiumFeaturesMetadataViewer::Construct(const FArguments& InArgs) {
  SAssignNew(this->_pContent, SVerticalBox);

  const TWeakObjectPtr<ACesium3DTileset>& pTileset = InArgs._Tileset;
  FString label =
      pTileset.IsValid() ? pTileset->GetActorLabel() : TEXT("Unknown");

  this->_pTileset = pTileset;
  this->Sync();

  SWindow::Construct(
      SWindow::FArguments()
          .Title(FText::FromString(FString::Format(
              TEXT("{0}: Features and Metadata Properties"),
              {label})))
          .AutoCenter(EAutoCenter::PreferredWorkArea)
          .SizingRule(ESizingRule::UserSized)
          .ClientSize(FVector2D(
              800,
              600))[SNew(SBorder)
                        .Visibility(EVisibility::Visible)
                        .BorderImage(FAppStyle::GetBrush("Menu.Background"))
                        .Padding(FMargin(10.0f))[this->_pContent->AsShared()]]);
}

namespace {
template <typename TEnum>
void populateEnumOptions(TArray<TSharedRef<TEnum>>& options) {
  UEnum* pEnum = StaticEnum<TEnum>();
  if (pEnum) {
    // "NumEnums" also includes the "_MAX" value, which indicates the number of
    // different values in the enum. Exclude it here.
    const int32 num = pEnum->NumEnums() - 1;
    options.Reserve(num);

    for (int32 i = 0; i < num; i++) {
      TEnum value = TEnum(pEnum->GetValueByIndex(i));
      options.Emplace(MakeShared<TEnum>(value));
    }
  }
}
} // namespace

void CesiumFeaturesMetadataViewer::Sync() {
  if (this->_conversionOptions.IsEmpty()) {
    populateEnumOptions<ECesiumEncodedMetadataConversion>(
        this->_conversionOptions);
  }
  if (this->_encodedTypeOptions.IsEmpty()) {
    populateEnumOptions<ECesiumEncodedMetadataType>(this->_encodedTypeOptions);
  }
  if (this->_encodedComponentTypeOptions.IsEmpty()) {
    populateEnumOptions<ECesiumEncodedMetadataComponentType>(
        this->_encodedComponentTypeOptions);
  }

  this->_metadataSources.Empty();
  this->_featureIdSets.Empty();
  this->_stringMap.Empty();

  this->gatherGltfFeaturesMetadata();

  TSharedRef<SVerticalBox> pContent = this->_pContent.ToSharedRef();
  pContent->ClearChildren();

  pContent->AddSlot().AutoHeight()
      [SNew(SHeader)
           .Content()[SNew(STextBlock)
                          .TextStyle(FCesiumEditorModule::GetStyle(), "Heading")
                          .Text(FText::FromString(TEXT("glTF Metadata")))
                          .Margin(FMargin(0.f, 10.f))]];

  if (!this->_metadataSources.IsEmpty()) {
    TSharedRef<SScrollBox> pGltfContent = SNew(SScrollBox);
    for (const PropertySourceView& source : this->_metadataSources) {
      this->createGltfPropertySourceDropdown(pGltfContent, source);
    }
    pContent->AddSlot().MaxHeight(400.0f).AutoHeight()[pGltfContent];
  } else {
    pContent->AddSlot().AutoHeight()
        [SNew(STextBlock)
             .AutoWrapText(true)
             .Text(FText::FromString(
                 TEXT("This tileset does not contain any glTF metadata.")))];
  }

  pContent->AddSlot().AutoHeight()
      [SNew(SHeader)
           .Content()[SNew(STextBlock)
                          .TextStyle(FCesiumEditorModule::GetStyle(), "Heading")
                          .Text(FText::FromString(TEXT("glTF Features")))
                          .Margin(FMargin(0.f, 10.f))]];

  if (!this->_featureIdSets.IsEmpty()) {
    TSharedRef<SScrollBox> pGltfFeatures = SNew(SScrollBox);
    for (const FeatureIdSetView& featureIdSet : this->_featureIdSets) {
      this->createGltfFeatureIdSetDropdown(pGltfFeatures, featureIdSet);
    }
    pContent->AddSlot().MaxHeight(400.0f).AutoHeight()[pGltfFeatures];
  } else {
    pContent->AddSlot().AutoHeight()
        [SNew(STextBlock)
             .AutoWrapText(true)
             .Text(FText::FromString(
                 TEXT("This tileset does not contain any glTF features.")))];
  }
}

namespace {
// These are copies of functions in EncodedFeaturesMetadata.h. The file is
// unfortunately too entangled in Private code to pull into Public.
FString getNameForPropertySource(const FCesiumPropertyTable& propertyTable) {
  FString propertyTableName =
      UCesiumPropertyTableBlueprintLibrary::GetPropertyTableName(propertyTable);

  if (propertyTableName.IsEmpty()) {
    // Substitute the name with the property table's class.
    propertyTableName = propertyTable.getClassName();
  }

  return propertyTableName;
}

FString
getNameForPropertySource(const FCesiumPropertyTexture& propertyTexture) {
  FString propertyTextureName =
      UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureName(
          propertyTexture);

  if (propertyTextureName.IsEmpty()) {
    // Substitute the name with the property texture's class.
    propertyTextureName = propertyTexture.getClassName();
  }

  return propertyTextureName;
}

FString getNameForFeatureIdSet(
    const FCesiumFeatureIdSet& featureIDSet,
    int32& featureIdTextureCounter) {
  FString label = UCesiumFeatureIdSetBlueprintLibrary::GetLabel(featureIDSet);
  if (!label.IsEmpty()) {
    return label;
  }

  ECesiumFeatureIdSetType type =
      UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDSetType(featureIDSet);

  if (type == ECesiumFeatureIdSetType::Attribute) {
    FCesiumFeatureIdAttribute attribute =
        UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDAttribute(
            featureIDSet);
    ECesiumFeatureIdAttributeStatus status =
        UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureIDAttributeStatus(
            attribute);
    if (status == ECesiumFeatureIdAttributeStatus::Valid) {
      std::string generatedName =
          "_FEATURE_ID_" + std::to_string(attribute.getAttributeIndex());
      return FString(generatedName.c_str());
    }
  }

  if (type == ECesiumFeatureIdSetType::Instance) {
    FCesiumFeatureIdAttribute attribute =
        UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDAttribute(
            featureIDSet);
    ECesiumFeatureIdAttributeStatus status =
        UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureIDAttributeStatus(
            attribute);
    if (status == ECesiumFeatureIdAttributeStatus::Valid) {
      std::string generatedName = "_FEATURE_INSTANCE_ID_" +
                                  std::to_string(attribute.getAttributeIndex());
      return FString(generatedName.c_str());
    }
  }

  if (type == ECesiumFeatureIdSetType::Texture) {
    std::string generatedName =
        "_FEATURE_ID_TEXTURE_" + std::to_string(featureIdTextureCounter);
    featureIdTextureCounter++;
    return FString(generatedName.c_str());
  }

  if (type == ECesiumFeatureIdSetType::Implicit) {
    return FString("_IMPLICIT_FEATURE_ID");
  }

  if (type == ECesiumFeatureIdSetType::InstanceImplicit) {
    return FString("_IMPLICIT_FEATURE_INSTANCE_ID");
  }

  // If for some reason an empty / invalid feature ID set was constructed,
  // return an empty name.
  return FString();
}
} // namespace

void CesiumFeaturesMetadataViewer::gatherGltfFeaturesMetadata() {
  if (!this->_pTileset.IsValid()) {
    return;
  }

  ACesium3DTileset& tileset = *this->_pTileset;

  for (const UActorComponent* pComponent : tileset.GetComponents()) {
    const auto* pPrimitive = Cast<UPrimitiveComponent>(pComponent);
    if (!pPrimitive) {
      continue;
    }

    const FCesiumModelMetadata& modelMetadata =
        UCesiumModelMetadataBlueprintLibrary::GetModelMetadata(pPrimitive);

    const TArray<FCesiumPropertyTable>& propertyTables =
        UCesiumModelMetadataBlueprintLibrary::GetPropertyTables(modelMetadata);
    this->gatherGltfPropertySources<
        FCesiumPropertyTable,
        UCesiumPropertyTableBlueprintLibrary,
        FCesiumPropertyTableProperty,
        UCesiumPropertyTablePropertyBlueprintLibrary>(propertyTables);

    const TArray<FCesiumPropertyTexture>& propertyTextures =
        UCesiumModelMetadataBlueprintLibrary::GetPropertyTextures(
            modelMetadata);
    this->gatherGltfPropertySources<
        FCesiumPropertyTexture,
        UCesiumPropertyTextureBlueprintLibrary,
        FCesiumPropertyTextureProperty,
        UCesiumPropertyTexturePropertyBlueprintLibrary>(propertyTextures);

    const FCesiumPrimitiveMetadata& primitiveMetadata =
        UCesiumPrimitiveMetadataBlueprintLibrary::GetPrimitiveMetadata(
            pPrimitive);

    const TArray<int64> propertyTextureIndices =
        UCesiumPrimitiveMetadataBlueprintLibrary::GetPropertyTextureIndices(
            primitiveMetadata);
    for (int64 propertyTextureIndex : propertyTextureIndices) {
      if (propertyTextureIndex < 0 ||
          propertyTextureIndex >= propertyTextures.Num()) {
        continue;
      }

      const FCesiumPropertyTexture& propertyTexture =
          propertyTextures[propertyTextureIndex];
      FString propertyTextureName = getNameForPropertySource(propertyTexture);
      this->_propertyTextureNames.Emplace(propertyTextureName);
    }

    const FCesiumPrimitiveFeatures& primitiveFeatures =
        UCesiumPrimitiveFeaturesBlueprintLibrary::GetPrimitiveFeatures(
            pPrimitive);

    const TArray<FCesiumFeatureIdSet>& featureIdSets =
        UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
            primitiveFeatures);

    for (const FCesiumFeatureIdSet& featureIdSet : featureIdSets) {
      ECesiumFeatureIdSetType type =
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDSetType(
              featureIdSet);
      int64 count =
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureCount(featureIdSet);
      if (type == ECesiumFeatureIdSetType::None || count == 0) {
        // Empty or invalid feature ID set. Skip.
        continue;
      }

      int32 featureIdTextureCounter = 0;
      FString name =
          getNameForFeatureIdSet(featureIdSet, featureIdTextureCounter);

      FeatureIdSetView* pFeatureIdSetView =
          this->_featureIdSets.FindByPredicate(
              [&name](const FeatureIdSetView& existing) {
                return *existing.pName == name;
              });

      if (!pFeatureIdSetView) {
        int32 index = this->_featureIdSets.Emplace(
            FeatureIdSetView{.pName = getSharedRef(name)});
        pFeatureIdSetView = &this->_featureIdSets[index];
      }

      const int64 propertyTableIndex =
          UCesiumFeatureIdSetBlueprintLibrary::GetPropertyTableIndex(
              featureIdSet);
      FString propertyTableName;
      if (propertyTables.IsValidIndex(propertyTableIndex)) {
        propertyTableName =
            getNameForPropertySource(propertyTables[propertyTableIndex]);
      }

      bool hasKhrTextureTransform = false;
      if (type == ECesiumFeatureIdSetType::Texture) {
        FCesiumFeatureIdTexture featureIdTexture =
            UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDTexture(
                featureIdSet);
        auto maybeTextureTransform =
            featureIdTexture.getFeatureIdTextureView().getTextureTransform();
        if (maybeTextureTransform) {
          hasKhrTextureTransform =
              (maybeTextureTransform->status() ==
               CesiumGltf::KhrTextureTransformStatus::Valid);
        }
      }

      FeatureIdSetInstance Instance{
          .pFeatureIdSetName = pFeatureIdSetView->pName,
          .type = type,
          .hasKhrTextureTransform = hasKhrTextureTransform,
          .pPropertyTableName = getSharedRef(propertyTableName)};

      TSharedRef<FeatureIdSetInstance>* pExistingInstance =
          pFeatureIdSetView->instances.FindByPredicate(
              [&Instance](const TSharedRef<FeatureIdSetInstance>& pExisting) {
                return Instance == *pExisting;
              });
      if (!pExistingInstance) {
        pFeatureIdSetView->instances.Emplace(
            MakeShared<FeatureIdSetInstance>(std::move(Instance)));
      }
    }
  }
}

bool CesiumFeaturesMetadataViewer::PropertyInstance::operator==(
    const PropertyInstance& rhs) const {
  if (*pPropertyId != *rhs.pPropertyId ||
      propertyDetails != rhs.propertyDetails) {
    return false;
  }

  if (sourceDetails.index() != rhs.sourceDetails.index()) {
    // Properties are different if they come from differently-typed sources.
    return false;
  }

  if (std::holds_alternative<TexturePropertyInstanceDetails>(sourceDetails)) {
    CESIUM_ASSERT(std::holds_alternative<TexturePropertyInstanceDetails>(
        rhs.sourceDetails));

    return std::get<TexturePropertyInstanceDetails>(sourceDetails)
               .hasKhrTextureTransform ==
           std::get<TexturePropertyInstanceDetails>(rhs.sourceDetails)
               .hasKhrTextureTransform;
  }

  return true;
}

bool CesiumFeaturesMetadataViewer::PropertyInstance::operator!=(
    const PropertyInstance& property) const {
  return !operator==(property);
}

bool CesiumFeaturesMetadataViewer::FeatureIdSetInstance::operator==(
    const FeatureIdSetInstance& rhs) const {
  if (*pFeatureIdSetName != *rhs.pFeatureIdSetName || type != rhs.type ||
      *pPropertyTableName != *rhs.pPropertyTableName) {
    return false;
  }

  if (type == ECesiumFeatureIdSetType::Texture) {
    return hasKhrTextureTransform == rhs.hasKhrTextureTransform;
  }

  return true;
}

bool CesiumFeaturesMetadataViewer::FeatureIdSetInstance::operator!=(
    const FeatureIdSetInstance& property) const {
  return !operator==(property);
}

namespace {

template <typename TEnum>
TArray<TSharedRef<TEnum>> getSharedRefs(
    const TArray<TSharedRef<TEnum>>& options,
    const TArray<TEnum>& selection) {
  TArray<TSharedRef<TEnum>> result;
  UEnum* pEnum = StaticEnum<TEnum>();
  if (!pEnum) {
    return result;
  }

  // Assumes populateEnumOptions will be initialized in enum order!
  for (TEnum value : selection) {
    int32 index = pEnum->GetIndexByValue(int64(value));
    CESIUM_ASSERT(index >= 0 && index < options.Num());
    result.Add(options[index]);
  }

  return result;
}

TArray<ECesiumEncodedMetadataConversion> getSupportedConversionsForProperty(
    const FCesiumMetadataPropertyDetails& PropertyDetails) {
  TArray<ECesiumEncodedMetadataConversion> result;
  if (PropertyDetails.Type == ECesiumMetadataType::Invalid) {
    return result;
  }

  result.Reserve(2);
  result.Add(ECesiumEncodedMetadataConversion::Coerce);

  if (PropertyDetails.Type == ECesiumMetadataType::String) {
    result.Add(ECesiumEncodedMetadataConversion::ParseColorFromString);
  }

  return result;
}
} // namespace

template <
    typename TSource,
    typename TSourceBlueprintLibrary,
    typename TSourceProperty,
    typename TSourcePropertyBlueprintLibrary>
void CesiumFeaturesMetadataViewer::gatherGltfPropertySources(
    const TArray<TSource>& sources) {

  for (const TSource& source : sources) {
    FString sourceName = getNameForPropertySource(source);
    TSharedRef<FString> pSourceName = this->getSharedRef(sourceName);

    constexpr EPropertySource sourceType =
        std::is_same_v<TSource, FCesiumPropertyTable>
            ? EPropertySource::PropertyTable
            : EPropertySource::PropertyTexture;

    PropertySourceView* pSource = this->_metadataSources.FindByPredicate(
        [&sourceName, sourceType](const PropertySourceView& existingSource) {
          return *existingSource.pName == sourceName &&
                 existingSource.type == sourceType;
        });

    if (!pSource) {
      int32 index = this->_metadataSources.Emplace(
          PropertySourceView{.pName = pSourceName, .type = sourceType});
      pSource = &this->_metadataSources[index];
    }

    const TMap<FString, TSourceProperty>& properties =
        TSourceBlueprintLibrary::GetProperties(source);
    for (const auto& propertyIt : properties) {
      TSharedRef<FString> pPropertyId = this->getSharedRef(propertyIt.Key);
      TSharedRef<PropertyView>* pProperty = pSource->properties.FindByPredicate(
          [&pPropertyId](const TSharedRef<PropertyView>& pExistingProperty) {
            return **pExistingProperty->pId == *pPropertyId;
          });

      if (!pProperty) {
        int32 index = pSource->properties.Emplace(MakeShared<PropertyView>(
            pPropertyId,
            TArray<TSharedRef<PropertyInstance>>()));
        pProperty = &pSource->properties[index];
      }

      PropertyView& property = **pProperty;

      FCesiumMetadataPropertyDetails propertyDetails;

      const FCesiumMetadataValueType valueType =
          TSourcePropertyBlueprintLibrary::GetValueType(propertyIt.Value);

      // Skip any invalid type properties.
      if (valueType.Type == ECesiumMetadataType::Invalid)
        continue;

      propertyDetails.Type = valueType.Type;
      propertyDetails.ComponentType = valueType.ComponentType;
      propertyDetails.bIsArray = valueType.bIsArray;
      propertyDetails.ArraySize =
          TSourcePropertyBlueprintLibrary::GetArraySize(propertyIt.Value);
      propertyDetails.bIsNormalized =
          TSourcePropertyBlueprintLibrary::IsNormalized(propertyIt.Value);

      FCesiumMetadataValue offset =
          TSourcePropertyBlueprintLibrary::GetOffset(propertyIt.Value);
      propertyDetails.bHasOffset =
          !UCesiumMetadataValueBlueprintLibrary::IsEmpty(offset);

      FCesiumMetadataValue scale =
          TSourcePropertyBlueprintLibrary::GetScale(propertyIt.Value);
      propertyDetails.bHasScale =
          !UCesiumMetadataValueBlueprintLibrary::IsEmpty(scale);

      FCesiumMetadataValue noData =
          TSourcePropertyBlueprintLibrary::GetNoDataValue(propertyIt.Value);
      propertyDetails.bHasNoDataValue =
          !UCesiumMetadataValueBlueprintLibrary::IsEmpty(scale);

      FCesiumMetadataValue defaultValue =
          TSourcePropertyBlueprintLibrary::GetDefaultValue(propertyIt.Value);
      propertyDetails.bHasDefaultValue =
          !UCesiumMetadataValueBlueprintLibrary::IsEmpty(scale);

      PropertyInstance instance{
          .pPropertyId = pPropertyId,
          .propertyDetails = std::move(propertyDetails),
          .pSourceName = pSourceName};

      if constexpr (std::is_same_v<
                        TSourceProperty,
                        FCesiumPropertyTableProperty>) {
        // Do some silly TSharedRef lookup since it's required by SComboBox.
        TArray<TSharedRef<ECesiumEncodedMetadataConversion>>
            supportedConversions = getSharedRefs(
                this->_conversionOptions,
                getSupportedConversionsForProperty(propertyDetails));
        instance.sourceDetails = TablePropertyInstanceDetails{
            .conversionMethods = std::move(supportedConversions)};
      } else if constexpr (std::is_same_v<
                               TSourceProperty,
                               FCesiumPropertyTextureProperty>) {
        auto maybeTextureTransform = propertyIt.Value.getTextureTransform();
        bool hasKhrTextureTransform =
            maybeTextureTransform &&
            (maybeTextureTransform->status() ==
             CesiumGltf::KhrTextureTransformStatus::Valid);
        instance.sourceDetails = TexturePropertyInstanceDetails{
            .hasKhrTextureTransform = hasKhrTextureTransform};
      }

      TSharedRef<PropertyInstance>* pExistingInstance =
          property.instances.FindByPredicate(
              [&instance](
                  const TSharedRef<PropertyInstance>& existingInstance) {
                return instance == *existingInstance;
              });
      if (!pExistingInstance) {
        property.instances.Emplace(
            MakeShared<PropertyInstance>(std::move(instance)));
      }
    }
  }
}

namespace {
template <typename TEnum> FString enumToNameString(TEnum value) {
  const UEnum* pEnum = StaticEnum<TEnum>();
  return pEnum ? pEnum->GetNameStringByValue((int64)value) : FString();
}
} // namespace

namespace {
template <typename TEnum> FText getEnumDisplayNameText(TEnum value) {
  UEnum* pEnum = StaticEnum<TEnum>();
  if (pEnum) {
    return pEnum->GetDisplayNameTextByValue(int64(value));
  }

  return FText::FromString(FString());
}
} // namespace

TSharedRef<ITableRow> CesiumFeaturesMetadataViewer::createPropertyInstanceRow(
    TSharedRef<PropertyInstance> pItem,
    const TSharedRef<STableViewBase>& list) {
  FString typeString = pItem->propertyDetails.GetValueType().ToString();
  if (pItem->propertyDetails.bIsNormalized) {
    typeString += TEXT(" (Normalized)");
  }

  if (pItem->propertyDetails.bIsArray) {
    int64 arraySize = pItem->propertyDetails.ArraySize;
    typeString += arraySize > 0
                      ? FString::Printf(TEXT(" with %d elements"), arraySize)
                      : TEXT(" of variable size");
  }

  auto* pTableSourceDetails =
      std::get_if<TablePropertyInstanceDetails>(&pItem->sourceDetails);
  auto* pTextureSourceDetails =
      std::get_if<TexturePropertyInstanceDetails>(&pItem->sourceDetails);

  TArray<FString> qualifierList;
  if (pItem->propertyDetails.bHasOffset) {
    qualifierList.Add("Offset");
  }
  if (pItem->propertyDetails.bHasScale) {
    qualifierList.Add("Scale");
  }
  if (pItem->propertyDetails.bHasNoDataValue) {
    qualifierList.Add("'No Data' Value");
  }
  if (pItem->propertyDetails.bHasDefaultValue) {
    qualifierList.Add("Default Value");
  }

  if (pTextureSourceDetails && pTextureSourceDetails->hasKhrTextureTransform) {
    qualifierList.Add("KHR_texture_transform");
  }

  FString qualifierString =
      qualifierList.IsEmpty()
          ? FString()
          : "Contains " + FString::Join(qualifierList, TEXT(", "));

  TSharedRef<SHorizontalBox> content =
      SNew(SHorizontalBox) +
      SHorizontalBox::Slot().FillWidth(0.45f).Padding(5.0f).VAlign(
          EVerticalAlignment::VAlign_Center)
          [SNew(STextBlock)
               .AutoWrapText(true)
               .Text(FText::FromString(typeString))
               .ToolTipText(FText::FromString(FString(
                   "The type of the property as defined in the EXT_structural_metadata extension.")))] +
      SHorizontalBox::Slot()
          .AutoWidth()
          .MaxWidth(1.0f)
          .Padding(5.0f)
          .HAlign(EHorizontalAlignment::HAlign_Left)
          .VAlign(EVerticalAlignment::VAlign_Center)
              [SNew(STextBlock)
                   .AutoWrapText(true)
                   .Text(FText::FromString(qualifierString))
                   .ToolTipText(FText::FromString(
                       "Notable qualities of the property that require additional nodes to be generated for the material."))];

  if (pTableSourceDetails) {
    FCesiumMetadataEncodingDetails bestFitEncodingDetails =
        FCesiumMetadataEncodingDetails::GetBestFitForProperty(
            pItem->propertyDetails);

    createEnumComboBox<ECesiumEncodedMetadataConversion>(
        pTableSourceDetails->pConversionCombo,
        pTableSourceDetails->conversionMethods,
        bestFitEncodingDetails.Conversion,
        TEXT(
            "The conversion method used to encode and send the property's data to the material."));
    createEnumComboBox<ECesiumEncodedMetadataType>(
        pTableSourceDetails->pEncodedTypeCombo,
        this->_encodedTypeOptions,
        bestFitEncodingDetails.Type,
        TEXT(
            "The type to which to coerce the property's data. Affects the texture format that is used to encode the data."));
    createEnumComboBox<ECesiumEncodedMetadataComponentType>(
        pTableSourceDetails->pEncodedComponentTypeCombo,
        this->_encodedComponentTypeOptions,
        bestFitEncodingDetails.ComponentType,
        TEXT(
            "The component type to which to coerce the property's data. Affects the texture format that is used to encode the data."));

    if (pTableSourceDetails->pConversionCombo.IsValid()) {
      content->AddSlot().FillWidth(0.65).Padding(5.0f).VAlign(
          EVerticalAlignment::VAlign_Center)
          [pTableSourceDetails->pConversionCombo->AsShared()];
    }

    auto visibilityLambda = TAttribute<EVisibility>::Create([pItem]() {
      const auto* pTableSourceDetails =
          std::get_if<TablePropertyInstanceDetails>(&pItem->sourceDetails);
      if (!pTableSourceDetails) {
        return EVisibility::Hidden;
      }

      bool show = false;
      if (pTableSourceDetails->pConversionCombo.IsValid()) {
        show =
            pTableSourceDetails->pConversionCombo->GetSelectedItem().IsValid();
      }
      return show ? EVisibility::Visible : EVisibility::Hidden;
    });

    if (pTableSourceDetails->pEncodedTypeCombo.IsValid()) {
      pTableSourceDetails->pEncodedTypeCombo->SetVisibility(visibilityLambda);
      content->AddSlot().AutoWidth().Padding(5.0f).VAlign(
          EVerticalAlignment::VAlign_Center)
          [pTableSourceDetails->pEncodedTypeCombo->AsShared()];
    }

    if (pTableSourceDetails->pEncodedComponentTypeCombo.IsValid()) {
      pTableSourceDetails->pEncodedComponentTypeCombo->SetVisibility(
          visibilityLambda);
      content->AddSlot().AutoWidth().Padding(5.0f).VAlign(
          EVerticalAlignment::VAlign_Center)
          [pTableSourceDetails->pEncodedComponentTypeCombo->AsShared()];
    }
  }

  content->AddSlot()
      .AutoWidth()
      .HAlign(EHorizontalAlignment::HAlign_Right)
      .VAlign(
          EVerticalAlignment::
              VAlign_Center)[PropertyCustomizationHelpers::MakeNewBlueprintButton(
          FSimpleDelegate::CreateLambda(
              [this, pItem]() { this->registerPropertyInstance(pItem); }),
          FText::FromString(TEXT(
              "Add this property to the tileset's CesiumFeaturesMetadataComponent.")),
          TAttribute<bool>::Create(
              [this, pItem]() { return this->canBeRegistered(pItem); }))];

  return SNew(STableRow<TSharedRef<PropertyInstance>>, list)
      .Content()[SNew(SBox)
                     .HAlign(EHorizontalAlignment::HAlign_Fill)
                     .Content()[content]];
}

TSharedRef<ITableRow> CesiumFeaturesMetadataViewer::createGltfPropertyDropdown(
    TSharedRef<PropertyView> pItem,
    const TSharedRef<STableViewBase>& list) {
  return SNew(STableRow<TSharedRef<PropertyView>>, list)
      .Content()[SNew(SExpandableArea)
                     .InitiallyCollapsed(true)
                     .HeaderContent()[SNew(STextBlock)
                                          .Text(FText::FromString(*pItem->pId))]
                     .BodyContent()[SNew(
                                        SListView<TSharedRef<PropertyInstance>>)
                                        .ListItemsSource(&pItem->instances)
                                        .SelectionMode(ESelectionMode::None)
                                        .OnGenerateRow(
                                            this,
                                            &CesiumFeaturesMetadataViewer::
                                                createPropertyInstanceRow)]];
}

void CesiumFeaturesMetadataViewer::createGltfPropertySourceDropdown(
    TSharedRef<SScrollBox>& pContent,
    const PropertySourceView& source) {
  FString sourceDisplayName = FString::Printf(TEXT("\"%s\""), **source.pName);
  switch (source.type) {
  case EPropertySource::PropertyTable:
    sourceDisplayName += " (Property Table)";
    break;
  case EPropertySource::PropertyTexture:
    sourceDisplayName += " (Property Texture)";
    break;
  }

  pContent->AddSlot()
      [SNew(SExpandableArea)
           .InitiallyCollapsed(true)
           .HeaderContent()[SNew(STextBlock)
                                .Text(FText::FromString(sourceDisplayName))]
           .BodyContent()[SNew(SListView<TSharedRef<PropertyView>>)
                              .ListItemsSource(&source.properties)
                              .SelectionMode(ESelectionMode::None)
                              .OnGenerateRow(
                                  this,
                                  &CesiumFeaturesMetadataViewer::
                                      createGltfPropertyDropdown)]];
}

template <typename TEnum>
TSharedRef<SWidget> CesiumFeaturesMetadataViewer::createEnumDropdownOption(
    TSharedRef<TEnum> pOption) {
  return SNew(STextBlock).Text(getEnumDisplayNameText(*pOption));
}

template <typename TEnum>
void CesiumFeaturesMetadataViewer::createEnumComboBox(
    TSharedPtr<SComboBox<TSharedRef<TEnum>>>& pComboBox,
    const TArray<TSharedRef<TEnum>>& options,
    TEnum initialValue,
    const FString& tooltip) {
  CESIUM_ASSERT(options.Num() > 0);

  int32 initialIndex = 0;
  for (int32 i = 0; i < options.Num(); i++) {
    if (initialValue == *options[i]) {
      initialIndex = i;
      break;
    }
  }

  SAssignNew(pComboBox, SComboBox<TSharedRef<TEnum>>)
      .OptionsSource(&options)
      .InitiallySelectedItem(options[initialIndex])
      .OnGenerateWidget(
          this,
          &CesiumFeaturesMetadataViewer::createEnumDropdownOption<TEnum>)
      .Content()
          [SNew(STextBlock).MinDesiredWidth(50.0f).Text_Lambda([&pComboBox]() {
            return pComboBox->GetSelectedItem().IsValid()
                       ? getEnumDisplayNameText<TEnum>(
                             *pComboBox->GetSelectedItem())
                       : FText::FromString(FString());
          })]
      .ToolTipText(FText::FromString(tooltip));
}

TSharedRef<ITableRow>
CesiumFeaturesMetadataViewer::createFeatureIdSetInstanceRow(
    TSharedRef<FeatureIdSetInstance> pItem,
    const TSharedRef<STableViewBase>& list) {
  FString sourceString = FString::Printf(
      TEXT("\"%s\" (Property Table)"),
      **pItem->pPropertyTableName);

  return SNew(STableRow<TSharedRef<FeatureIdSetInstance>>, list)
      .Content()
          [SNew(SBox)
               .HAlign(EHorizontalAlignment::HAlign_Fill)
               .Content()
                   [SNew(SHorizontalBox) +
                    SHorizontalBox::Slot().FillWidth(0.5f).Padding(5.0f).VAlign(
                        EVerticalAlignment::VAlign_Center)
                        [SNew(STextBlock)
                             .AutoWrapText(true)
                             .Text(FText::FromString(
                                 enumToNameString(pItem->type)))] +
                    SHorizontalBox::Slot().FillWidth(1.0f).Padding(5.0f).VAlign(
                        EVerticalAlignment::VAlign_Center)
                        [SNew(STextBlock)
                             .AutoWrapText(true)
                             .Text(FText::FromString(sourceString))] +
                    SHorizontalBox::Slot()
                        .AutoWidth()
                        .HAlign(EHorizontalAlignment::HAlign_Right)
                        .VAlign(EVerticalAlignment::VAlign_Center)
                            [PropertyCustomizationHelpers::MakeNewBlueprintButton(
                                FSimpleDelegate::CreateLambda([this, pItem]() {
                                  this->registerFeatureIdSetInstance(pItem);
                                }),
                                FText::FromString(TEXT(
                                    "Add this property statistic to the tileset's CesiumFeaturesMetadataComponent.")),
                                TAttribute<bool>::Create([this, pItem]() {
                                  return this->canBeRegistered(pItem);
                                }))]]];
}

void CesiumFeaturesMetadataViewer::createGltfFeatureIdSetDropdown(
    TSharedRef<SScrollBox>& pContent,
    const FeatureIdSetView& featureIdSet) {
  pContent->AddSlot()
      [SNew(SExpandableArea)
           .InitiallyCollapsed(true)
           .HeaderContent()[SNew(STextBlock)
                                .Text(FText::FromString(*featureIdSet.pName))]
           .BodyContent()[SNew(SListView<TSharedRef<FeatureIdSetInstance>>)
                              .ListItemsSource(&featureIdSet.instances)
                              .SelectionMode(ESelectionMode::None)
                              .OnGenerateRow(
                                  this,
                                  &CesiumFeaturesMetadataViewer::
                                      createFeatureIdSetInstanceRow)]];
}

namespace {
template <typename TPropertySource, typename TProperty>
TProperty* findProperty(
    TArray<TPropertySource>& sources,
    const FString& sourceName,
    const FString& propertyName,
    bool createIfMissing) {
  TPropertySource* pPropertySource = sources.FindByPredicate(
      [&sourceName](const TPropertySource& existingSource) {
        return sourceName == existingSource.Name;
      });
  if (!pPropertySource && !createIfMissing) {
    return nullptr;
  }

  if (!pPropertySource) {
    int32 index = sources.Emplace(sourceName, TArray<TProperty>());
    pPropertySource = &sources[index];
  }

  TProperty* pProperty = pPropertySource->Properties.FindByPredicate(
      [&propertyName](const TProperty& existingProperty) {
        return propertyName == existingProperty.Name;
      });

  if (!pProperty && createIfMissing) {
    int32 index = pPropertySource->Properties.Emplace();
    pProperty = &pPropertySource->Properties[index];
    pProperty->Name = propertyName;
  }

  return pProperty;
}

FCesiumFeatureIdSetDescription* findFeatureIdSet(
    TArray<FCesiumFeatureIdSetDescription>& featureIdSets,
    const FString& name,
    bool createIfMissing) {
  FCesiumFeatureIdSetDescription* pFeatureIdSet = featureIdSets.FindByPredicate(
      [&name](const FCesiumFeatureIdSetDescription& existingSet) {
        return name == existingSet.Name;
      });

  if (!pFeatureIdSet && createIfMissing) {
    int32 index = featureIdSets.Emplace();
    pFeatureIdSet = &featureIdSets[index];
    pFeatureIdSet->Name = name;
  }

  return pFeatureIdSet;
}
} // namespace

namespace {
FCesiumMetadataEncodingDetails getSelectedEncodingDetails(
    const TSharedPtr<SComboBox<TSharedRef<ECesiumEncodedMetadataConversion>>>&
        pConversionCombo,
    const TSharedPtr<SComboBox<TSharedRef<ECesiumEncodedMetadataType>>>&
        pEncodedTypeCombo,
    const TSharedPtr<
        SComboBox<TSharedRef<ECesiumEncodedMetadataComponentType>>>&
        pEncodedComponentTypeCombo) {
  if (!pConversionCombo || !pEncodedTypeCombo || !pEncodedComponentTypeCombo)
    return FCesiumMetadataEncodingDetails();

  TSharedPtr<ECesiumEncodedMetadataConversion> pConversion =
      pConversionCombo->GetSelectedItem();
  TSharedPtr<ECesiumEncodedMetadataType> pEncodedType =
      pEncodedTypeCombo->GetSelectedItem();
  TSharedPtr<ECesiumEncodedMetadataComponentType> pEncodedComponentType =
      pEncodedComponentTypeCombo->GetSelectedItem();

  return FCesiumMetadataEncodingDetails(
      pEncodedType.IsValid() ? *pEncodedType : ECesiumEncodedMetadataType::None,
      pEncodedComponentType.IsValid()
          ? *pEncodedComponentType
          : ECesiumEncodedMetadataComponentType::None,
      pConversion.IsValid() ? *pConversion
                            : ECesiumEncodedMetadataConversion::None);
}
} // namespace

bool CesiumFeaturesMetadataViewer::canBeRegistered(
    TSharedRef<PropertyInstance> pItem) {
  if (pItem->propertyDetails.Type == ECesiumMetadataType::Invalid) {
    return false;
  }

  if (!this->_pFeaturesMetadataComponent.IsValid()) {
    return false;
  }

  UCesiumFeaturesMetadataComponent& featuresMetadata =
      *this->_pFeaturesMetadataComponent;

  if (std::holds_alternative<TablePropertyInstanceDetails>(
          pItem->sourceDetails)) {
    const auto& sourceDetails =
        std::get<TablePropertyInstanceDetails>(pItem->sourceDetails);

    // Validate encoding details first.
    FCesiumMetadataEncodingDetails selectedEncodingDetails =
        getSelectedEncodingDetails(
            sourceDetails.pConversionCombo,
            sourceDetails.pEncodedTypeCombo,
            sourceDetails.pEncodedComponentTypeCombo);

    switch (selectedEncodingDetails.Conversion) {
    case ECesiumEncodedMetadataConversion::Coerce:
    case ECesiumEncodedMetadataConversion::ParseColorFromString:
      // Ensure that we're coercing to a valid type.
      if (!selectedEncodingDetails.HasValidType())
        return false;
      else
        break;
    case ECesiumEncodedMetadataConversion::None:
    default:
      return false;
    }

    // Then, check whether the property already exists with the same
    // information.
    FCesiumPropertyTablePropertyDescription* pProperty = findProperty<
        FCesiumPropertyTableDescription,
        FCesiumPropertyTablePropertyDescription>(
        featuresMetadata.Description.ModelMetadata.PropertyTables,
        *pItem->pSourceName,
        *pItem->pPropertyId,
        false);

    return !pProperty || pProperty->PropertyDetails != pItem->propertyDetails ||
           pProperty->EncodingDetails != selectedEncodingDetails;
  }

  if (std::holds_alternative<TexturePropertyInstanceDetails>(
          pItem->sourceDetails)) {
    const auto& sourceDetails =
        std::get<TexturePropertyInstanceDetails>(pItem->sourceDetails);

    FCesiumPropertyTexturePropertyDescription* pProperty = findProperty<
        FCesiumPropertyTextureDescription,
        FCesiumPropertyTexturePropertyDescription>(
        featuresMetadata.Description.ModelMetadata.PropertyTextures,
        *pItem->pSourceName,
        *pItem->pPropertyId,
        false);

    return !pProperty || pProperty->PropertyDetails != pItem->propertyDetails ||
           pProperty->bHasKhrTextureTransform !=
               sourceDetails.hasKhrTextureTransform;
  }

  return false;
}

bool CesiumFeaturesMetadataViewer::canBeRegistered(
    TSharedRef<FeatureIdSetInstance> pItem) {
  if (pItem->type == ECesiumFeatureIdSetType::None) {
    return false;
  }

  if (!this->_pFeaturesMetadataComponent.IsValid()) {
    return false;
  }

  UCesiumFeaturesMetadataComponent& featuresMetadata =
      *this->_pFeaturesMetadataComponent;
  FCesiumFeatureIdSetDescription* pFeatureIdSet = findFeatureIdSet(
      featuresMetadata.Description.PrimitiveFeatures.FeatureIdSets,
      *pItem->pFeatureIdSetName,
      false);

  return !pFeatureIdSet ||
         pFeatureIdSet->PropertyTableName != *pItem->pPropertyTableName;
}

void CesiumFeaturesMetadataViewer::registerPropertyInstance(
    TSharedRef<PropertyInstance> pItem) {
  if (!this->_pFeaturesMetadataComponent.IsValid()) {
    UE_LOG(
        LogCesiumEditor,
        Error,
        TEXT(
            "This window was opened for a now invalid CesiumFeaturesMetadataComponent."))
    return;
  }

  UCesiumFeaturesMetadataComponent& featuresMetadata =
      *this->_pFeaturesMetadataComponent;

  featuresMetadata.PreEditChange(NULL);

  if (std::holds_alternative<TablePropertyInstanceDetails>(
          pItem->sourceDetails)) {
    CESIUM_ASSERT(
        pItem->pConversionCombo && pItem->pEncodedTypeCombo &&
        pItem->pEncodedComponentTypeCombo);

    FCesiumPropertyTablePropertyDescription* pProperty = findProperty<
        FCesiumPropertyTableDescription,
        FCesiumPropertyTablePropertyDescription>(
        featuresMetadata.Description.ModelMetadata.PropertyTables,
        *pItem->pSourceName,
        *pItem->pPropertyId,
        true);

    CESIUM_ASSERT(pProperty != nullptr);

    const auto& sourceDetails =
        std::get<TablePropertyInstanceDetails>(pItem->sourceDetails);

    FCesiumPropertyTablePropertyDescription& property = *pProperty;
    property.PropertyDetails = pItem->propertyDetails;
    property.EncodingDetails = getSelectedEncodingDetails(
        sourceDetails.pConversionCombo,
        sourceDetails.pEncodedTypeCombo,
        sourceDetails.pEncodedComponentTypeCombo);
  } else if (std::holds_alternative<TexturePropertyInstanceDetails>(
                 pItem->sourceDetails)) {
    FCesiumPropertyTexturePropertyDescription* pProperty = findProperty<
        FCesiumPropertyTextureDescription,
        FCesiumPropertyTexturePropertyDescription>(
        featuresMetadata.Description.ModelMetadata.PropertyTextures,
        *pItem->pSourceName,
        *pItem->pPropertyId,
        true);

    CESIUM_ASSERT(pProperty != nullptr);

    const auto& sourceDetails =
        std::get<TexturePropertyInstanceDetails>(pItem->sourceDetails);

    FCesiumPropertyTexturePropertyDescription& property = *pProperty;
    property.PropertyDetails = pItem->propertyDetails;
    property.bHasKhrTextureTransform = sourceDetails.hasKhrTextureTransform;
  }

  featuresMetadata.PostEditChange();
}

void CesiumFeaturesMetadataViewer::registerFeatureIdSetInstance(
    TSharedRef<FeatureIdSetInstance> pItem) {
  if (!this->_pFeaturesMetadataComponent.IsValid()) {
    UE_LOG(
        LogCesiumEditor,
        Error,
        TEXT(
            "This window was opened for a now invalid CesiumFeaturesMetadataComponent."))
    return;
  }

  UCesiumFeaturesMetadataComponent& featuresMetadata =
      *this->_pFeaturesMetadataComponent;

  featuresMetadata.PreEditChange(NULL);

  FCesiumFeatureIdSetDescription* pFeatureIdSet = findFeatureIdSet(
      featuresMetadata.Description.PrimitiveFeatures.FeatureIdSets,
      *pItem->pFeatureIdSetName,
      true);
  CESIUM_ASSERT(pFeatureIdSet != nullptr);

  pFeatureIdSet->Type = pItem->type;
  pFeatureIdSet->bHasKhrTextureTransform = pItem->hasKhrTextureTransform;
  pFeatureIdSet->PropertyTableName = *pItem->pPropertyTableName;

  featuresMetadata.PostEditChange();
}

TSharedRef<FString>
CesiumFeaturesMetadataViewer::getSharedRef(const FString& string) {
  return this->_stringMap.Contains(string)
             ? this->_stringMap[string]
             : this->_stringMap.Emplace(string, MakeShared<FString>(string));
}
