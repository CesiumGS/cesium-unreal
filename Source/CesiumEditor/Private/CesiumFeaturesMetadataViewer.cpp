// Copyright 2020-2025 CesiumGS, Inc. and Contributors

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
/*static*/ TArray<TSharedRef<ECesiumEncodedMetadataConversion>>
    CesiumFeaturesMetadataViewer::_conversionOptions = {};
/*static*/ TArray<TSharedRef<ECesiumEncodedMetadataType>>
    CesiumFeaturesMetadataViewer::_encodedTypeOptions = {};
/*static*/ TArray<TSharedRef<ECesiumEncodedMetadataComponentType>>
    CesiumFeaturesMetadataViewer::_encodedComponentTypeOptions = {};
/*static*/ TMap<FString, TSharedRef<FString>>
    CesiumFeaturesMetadataViewer::_stringMap = {};

/*static*/ void
CesiumFeaturesMetadataViewer::Open(TWeakObjectPtr<ACesium3DTileset> pTileset) {
  if (_pExistingWindow.IsValid()) {
    _pExistingWindow->_pTileset = pTileset;
    _pExistingWindow->SyncAndRebuildUI();
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

  _pExistingWindow->BringToFront();
}

void CesiumFeaturesMetadataViewer::Construct(const FArguments& InArgs) {
  CesiumFeaturesMetadataViewer::initializeStaticVariables();

  SAssignNew(this->_pContent, SVerticalBox);

  const TWeakObjectPtr<ACesium3DTileset>& pTileset = InArgs._Tileset;
  FString label =
      pTileset.IsValid() ? pTileset->GetActorLabel() : TEXT("Unknown");

  this->_pTileset = pTileset;
  this->SyncAndRebuildUI();

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

void CesiumFeaturesMetadataViewer::SyncAndRebuildUI() {
  this->_metadataSources.Empty();
  this->_featureIdSets.Empty();
  this->_stringMap.Empty();
  this->_propertyTextureNames.Empty();

  this->gatherGltfFeaturesMetadata();

  TSharedRef<SVerticalBox> pContent = this->_pContent.ToSharedRef();
  pContent->ClearChildren();

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
        [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(0.05f) +
         SHorizontalBox::Slot()
             [SNew(STextBlock)
                  .AutoWrapText(true)
                  .Text(FText::FromString(TEXT(
                      "This tileset does not contain any glTF features in this view.")))]];
  }

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
        [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(0.05f) +
         SHorizontalBox::Slot()
             [SNew(STextBlock)
                  .AutoWrapText(true)
                  .Text(FText::FromString(TEXT(
                      "This tileset does not contain any glTF metadata in this view.")))]];
  }

  pContent->AddSlot()
      .AutoHeight()
      .Padding(0.0f, 10.0f)
      .VAlign(VAlign_Bottom)
      .HAlign(HAlign_Center)
          [SNew(SButton)
               .ButtonStyle(FCesiumEditorModule::GetStyle(), "CesiumButton")
               .TextStyle(FCesiumEditorModule::GetStyle(), "CesiumButtonText")
               .ContentPadding(FMargin(1.0, 1.0))
               .HAlign(EHorizontalAlignment::HAlign_Center)
               .Text(FText::FromString(TEXT("Refresh with Current View")))
               .ToolTipText(FText::FromString(TEXT(
                   "Refreshes the lists with the feature ID sets and metadata from currently loaded tiles in the ACesium3DTileset.")))
               .OnClicked_Lambda([this]() {
                 this->SyncAndRebuildUI();
                 return FReply::Handled();
               })];
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
      if (!propertyTextures.IsValidIndex(propertyTextureIndex)) {
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

    int32 featureIdTextureCounter = 0;

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

      FeatureIdSetInstance Instance{
          .pFeatureIdSetName = pFeatureIdSetView->pName,
          .type = type,
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

  if (encodingDetails.has_value() != rhs.encodingDetails.has_value()) {
    return false;
  } else if (encodingDetails) {
    return encodingDetails->conversionMethods ==
           rhs.encodingDetails->conversionMethods;
  }

  return true;
}

bool CesiumFeaturesMetadataViewer::PropertyInstance::operator!=(
    const PropertyInstance& property) const {
  return !operator==(property);
}

bool CesiumFeaturesMetadataViewer::FeatureIdSetInstance::operator==(
    const FeatureIdSetInstance& rhs) const {
  return *pFeatureIdSetName == *rhs.pFeatureIdSetName && type == rhs.type &&
         *pPropertyTableName == *rhs.pPropertyTableName;
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
        PropertyView newProperty{.pId = pPropertyId, .instances = {}};
        int32 index = pSource->properties.Emplace(
            MakeShared<PropertyView>(std::move(newProperty)));
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
        instance.encodingDetails = PropertyInstanceEncodingDetails{
            .conversionMethods = std::move(supportedConversions)};
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

  if (pItem->encodingDetails) {
    FCesiumMetadataEncodingDetails bestFitEncodingDetails =
        FCesiumMetadataEncodingDetails::GetBestFitForProperty(
            pItem->propertyDetails);

    createEnumComboBox<ECesiumEncodedMetadataConversion>(
        pItem->encodingDetails->pConversionCombo,
        pItem->encodingDetails->conversionMethods,
        bestFitEncodingDetails.Conversion,
        FString());

    createEnumComboBox<ECesiumEncodedMetadataType>(
        pItem->encodingDetails->pEncodedTypeCombo,
        this->_encodedTypeOptions,
        bestFitEncodingDetails.Type,
        TEXT(
            "The type to which to coerce the property's data. Affects the texture format that is used to encode the data."));
    createEnumComboBox<ECesiumEncodedMetadataComponentType>(
        pItem->encodingDetails->pEncodedComponentTypeCombo,
        this->_encodedComponentTypeOptions,
        bestFitEncodingDetails.ComponentType,
        TEXT(
            "The component type to which to coerce the property's data. Affects the texture format that is used to encode the data."));

    if (pItem->encodingDetails->pConversionCombo.IsValid()) {
      content->AddSlot().FillWidth(0.65).Padding(5.0f).VAlign(
          EVerticalAlignment::VAlign_Center)
          [pItem->encodingDetails->pConversionCombo->AsShared()];
    }

    auto visibilityLambda = TAttribute<EVisibility>::Create([pItem]() {
      if (!pItem->encodingDetails) {
        return EVisibility::Hidden;
      }

      bool show = false;
      if (pItem->encodingDetails->pConversionCombo.IsValid()) {
        show = pItem->encodingDetails->pConversionCombo->GetSelectedItem()
                   .IsValid();
      }
      return show ? EVisibility::Visible : EVisibility::Hidden;
    });

    if (pItem->encodingDetails->pEncodedTypeCombo.IsValid()) {
      pItem->encodingDetails->pEncodedTypeCombo->SetVisibility(
          visibilityLambda);
      content->AddSlot().AutoWidth().Padding(5.0f).VAlign(
          EVerticalAlignment::VAlign_Center)
          [pItem->encodingDetails->pEncodedTypeCombo->AsShared()];
    }

    if (pItem->encodingDetails->pEncodedComponentTypeCombo.IsValid()) {
      pItem->encodingDetails->pEncodedComponentTypeCombo->SetVisibility(
          visibilityLambda);
      content->AddSlot().AutoWidth().Padding(5.0f).VAlign(
          EVerticalAlignment::VAlign_Center)
          [pItem->encodingDetails->pEncodedComponentTypeCombo->AsShared()];
    }
  }

  TSharedRef<SWidget> pAddButton = PropertyCustomizationHelpers::MakeAddButton(
      FSimpleDelegate::CreateLambda(
          [this, pItem]() { this->registerPropertyInstance(pItem); }),
      FText::FromString(TEXT(
          "Add this property to the tileset's CesiumFeaturesMetadataComponent.")),
      TAttribute<bool>::Create(
          [this, pItem]() { return this->canBeRegistered(pItem); }));
  pAddButton->SetVisibility(TAttribute<EVisibility>::Create([this, pItem]() {
    return this->canBeRegistered(pItem) ? EVisibility::Visible
                                        : EVisibility::Collapsed;
  }));

  TSharedRef<SWidget> pRemoveButton = PropertyCustomizationHelpers::MakeRemoveButton(
      FSimpleDelegate::CreateLambda(
          [this, pItem]() { this->removePropertyInstance(pItem); }),
      FText::FromString(TEXT(
          "Remove this property from the tileset's CesiumFeaturesMetadataComponent.")),
      TAttribute<bool>::Create(
          [this, pItem]() { return !this->canBeRegistered(pItem); }));
  pRemoveButton->SetVisibility(TAttribute<EVisibility>::Create([this, pItem]() {
    return this->canBeRegistered(pItem) ? EVisibility::Collapsed
                                        : EVisibility::Visible;
  }));

  content->AddSlot()
      .AutoWidth()
      .HAlign(EHorizontalAlignment::HAlign_Right)
      .VAlign(EVerticalAlignment::VAlign_Center)
          [SNew(SHorizontalBox) +
           SHorizontalBox::Slot().AutoWidth()[pAddButton] +
           SHorizontalBox::Slot().AutoWidth()[pRemoveButton]];

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
           .InitiallyCollapsed(false)
           .HeaderContent()[SNew(STextBlock)
                                .Text(FText::FromString(sourceDisplayName))]
           .BodyContent()
               [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(0.05f) +
                SHorizontalBox::Slot()
                    [SNew(SListView<TSharedRef<PropertyView>>)
                         .ListItemsSource(&source.properties)
                         .SelectionMode(ESelectionMode::None)
                         .OnGenerateRow(
                             this,
                             &CesiumFeaturesMetadataViewer::
                                 createGltfPropertyDropdown)]]];
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
      .Content()[SNew(STextBlock)
                     .MinDesiredWidth(50.0f)
                     .Text_Lambda([&pComboBox]() {
                       return pComboBox->GetSelectedItem().IsValid()
                                  ? getEnumDisplayNameText<TEnum>(
                                        *pComboBox->GetSelectedItem())
                                  : FText::FromString(FString());
                     })
                     .ToolTipText_Lambda([&pComboBox, tooltip]() {
                       if constexpr (std::is_same_v<
                                         TEnum,
                                         ECesiumEncodedMetadataConversion>) {
                         UEnum* pEnum =
                             StaticEnum<ECesiumEncodedMetadataConversion>();
                         if (pEnum) {
                           return pComboBox->GetSelectedItem().IsValid()
                                      ? pEnum->GetToolTipTextByIndex(int64(
                                            *pComboBox->GetSelectedItem()))
                                      : FText::FromString(FString());
                         }
                       }
                       return FText::FromString(tooltip);
                     })];
}

TSharedRef<ITableRow>
CesiumFeaturesMetadataViewer::createFeatureIdSetInstanceRow(
    TSharedRef<FeatureIdSetInstance> pItem,
    const TSharedRef<STableViewBase>& list) {
  TSharedRef<SHorizontalBox> pBox =
      SNew(SHorizontalBox) +
      SHorizontalBox::Slot().FillWidth(0.5f).Padding(5.0f).VAlign(
          EVerticalAlignment::VAlign_Center)
          [SNew(STextBlock)
               .AutoWrapText(true)
               .Text(FText::FromString(enumToNameString(pItem->type)))];

  if (!pItem->pPropertyTableName->IsEmpty()) {
    FString sourceString = FString::Printf(
        TEXT("Used with \"%s\" (Property Table)"),
        **pItem->pPropertyTableName);
    pBox->AddSlot()
        .FillWidth(1.0f)
        .Padding(5.0f)
        .HAlign(HAlign_Fill)
        .VAlign(EVerticalAlignment::VAlign_Center)
            [SNew(STextBlock)
                 .AutoWrapText(true)
                 .Text(FText::FromString(sourceString))
                 .ToolTipText(FText::FromString(
                     "The property table with which this feature ID set should be used. "
                     "Add properties from the corresponding property table under \"glTF Metadata\"."))];
  }

  TSharedRef<SWidget> pAddButton = PropertyCustomizationHelpers::MakeAddButton(
      FSimpleDelegate::CreateLambda(
          [this, pItem]() { this->registerFeatureIdSetInstance(pItem); }),
      FText::FromString(TEXT(
          "Add this feature ID set to the tileset's CesiumFeaturesMetadataComponent.")),
      TAttribute<bool>::Create(
          [this, pItem]() { return this->canBeRegistered(pItem); }));
  pAddButton->SetVisibility(TAttribute<EVisibility>::Create([this, pItem]() {
    return this->canBeRegistered(pItem) ? EVisibility::Visible
                                        : EVisibility::Collapsed;
  }));

  TSharedRef<SWidget> pRemoveButton = PropertyCustomizationHelpers::MakeRemoveButton(
      FSimpleDelegate::CreateLambda(
          [this, pItem]() { this->removeFeatureIdSetInstance(pItem); }),
      FText::FromString(TEXT(
          "Remove this feature ID set from the tileset's CesiumFeaturesMetadataComponent.")),
      TAttribute<bool>::Create(
          [this, pItem]() { return !this->canBeRegistered(pItem); }));
  pRemoveButton->SetVisibility(TAttribute<EVisibility>::Create([this, pItem]() {
    return this->canBeRegistered(pItem) ? EVisibility::Collapsed
                                        : EVisibility::Visible;
  }));

  pBox->AddSlot()
      .AutoWidth()
      .HAlign(EHorizontalAlignment::HAlign_Right)
      .VAlign(EVerticalAlignment::VAlign_Center)
          [SNew(SHorizontalBox) +
           SHorizontalBox::Slot().AutoWidth()[pAddButton] +
           SHorizontalBox::Slot().AutoWidth()[pRemoveButton]];

  return SNew(STableRow<TSharedRef<FeatureIdSetInstance>>, list)
      .Content()[SNew(SBox)
                     .HAlign(EHorizontalAlignment::HAlign_Fill)
                     .Content()[std::move(pBox)]];
}

void CesiumFeaturesMetadataViewer::createGltfFeatureIdSetDropdown(
    TSharedRef<SScrollBox>& pContent,
    const FeatureIdSetView& featureIdSet) {
  pContent->AddSlot()
      [SNew(SExpandableArea)
           .InitiallyCollapsed(false)
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
    int32 index =
        sources.Emplace(TPropertySource{sourceName, TArray<TProperty>()});
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

  if (pItem->encodingDetails) {
    // Validate encoding details first.
    FCesiumMetadataEncodingDetails selectedEncodingDetails =
        getSelectedEncodingDetails(
            pItem->encodingDetails->pConversionCombo,
            pItem->encodingDetails->pEncodedTypeCombo,
            pItem->encodingDetails->pEncodedComponentTypeCombo);

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
  } else {
    FCesiumPropertyTexturePropertyDescription* pProperty = findProperty<
        FCesiumPropertyTextureDescription,
        FCesiumPropertyTexturePropertyDescription>(
        featuresMetadata.Description.ModelMetadata.PropertyTextures,
        *pItem->pSourceName,
        *pItem->pPropertyId,
        false);

    return !pProperty || pProperty->PropertyDetails != pItem->propertyDetails;
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

  UKismetSystemLibrary::BeginTransaction(
      TEXT("Cesium Features / Metadata Viewer"),
      FText::FromString(
          FString("Register property instance with ACesium3DTileset")),
      this->_pFeaturesMetadataComponent.Get());
  this->_pFeaturesMetadataComponent->PreEditChange(NULL);

  FCesiumFeaturesMetadataDescription& description =
      this->_pFeaturesMetadataComponent->Description;

  if (pItem->encodingDetails) {
    CESIUM_ASSERT(
        pItem->encodingDetails->pConversionCombo &&
        pItem->encodingDetails->pEncodedTypeCombo &&
        pItem->encodingDetails->pEncodedComponentTypeCombo);

    FCesiumPropertyTablePropertyDescription* pProperty = findProperty<
        FCesiumPropertyTableDescription,
        FCesiumPropertyTablePropertyDescription>(
        description.ModelMetadata.PropertyTables,
        *pItem->pSourceName,
        *pItem->pPropertyId,
        true);

    CESIUM_ASSERT(pProperty != nullptr);

    FCesiumPropertyTablePropertyDescription& property = *pProperty;
    property.PropertyDetails = pItem->propertyDetails;
    property.EncodingDetails = getSelectedEncodingDetails(
        pItem->encodingDetails->pConversionCombo,
        pItem->encodingDetails->pEncodedTypeCombo,
        pItem->encodingDetails->pEncodedComponentTypeCombo);
  } else {
    FCesiumPropertyTexturePropertyDescription* pProperty = findProperty<
        FCesiumPropertyTextureDescription,
        FCesiumPropertyTexturePropertyDescription>(
        description.ModelMetadata.PropertyTextures,
        *pItem->pSourceName,
        *pItem->pPropertyId,
        true);

    CESIUM_ASSERT(pProperty != nullptr);

    FCesiumPropertyTexturePropertyDescription& property = *pProperty;
    property.PropertyDetails = pItem->propertyDetails;

    if (!this->_propertyTextureNames.Contains(*pItem->pSourceName)) {
      description.PrimitiveMetadata.PropertyTextureNames.Add(
          *pItem->pSourceName);
    }
  }

  this->_pFeaturesMetadataComponent->PostEditChange();
  UKismetSystemLibrary::EndTransaction();
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

  UKismetSystemLibrary::BeginTransaction(
      TEXT("Cesium Features / Metadata Viewer"),
      FText::FromString(
          FString("Register feature ID set instance with ACesium3DTileset")),
      this->_pFeaturesMetadataComponent.Get());
  this->_pFeaturesMetadataComponent->PreEditChange(NULL);

  FCesiumFeaturesMetadataDescription& description =
      this->_pFeaturesMetadataComponent->Description;

  FCesiumFeatureIdSetDescription* pFeatureIdSet = findFeatureIdSet(
      description.PrimitiveFeatures.FeatureIdSets,
      *pItem->pFeatureIdSetName,
      true);
  CESIUM_ASSERT(pFeatureIdSet != nullptr);

  pFeatureIdSet->Type = pItem->type;
  pFeatureIdSet->PropertyTableName = *pItem->pPropertyTableName;

  this->_pFeaturesMetadataComponent->PostEditChange();
  UKismetSystemLibrary::EndTransaction();
}

void CesiumFeaturesMetadataViewer::removePropertyInstance(
    TSharedRef<PropertyInstance> pItem) {
  if (!this->_pFeaturesMetadataComponent.IsValid()) {
    UE_LOG(
        LogCesiumEditor,
        Error,
        TEXT(
            "This window was opened for a now invalid CesiumFeaturesMetadataComponent."))
    return;
  }

  FCesiumFeaturesMetadataDescription& description =
      this->_pFeaturesMetadataComponent->Description;

  if (pItem->encodingDetails) {
    TArray<FCesiumPropertyTableDescription>& propertyTables =
        description.ModelMetadata.PropertyTables;

    int32 tableIndex = INDEX_NONE;
    for (int32 i = 0; i < propertyTables.Num(); i++) {
      if (propertyTables[i].Name == *pItem->pSourceName) {
        tableIndex = i;
        break;
      }
    }
    if (tableIndex == INDEX_NONE) {
      return;
    }
    FCesiumPropertyTableDescription& propertyTable = propertyTables[tableIndex];

    int32 propertyIndex = INDEX_NONE;
    for (int32 i = 0; i < propertyTable.Properties.Num(); i++) {
      if (propertyTable.Properties[i].Name == *pItem->pPropertyId) {
        propertyIndex = i;
        break;
      }
    }

    if (propertyIndex != INDEX_NONE) {
      UKismetSystemLibrary::BeginTransaction(
          TEXT("Cesium Features / Metadata Viewer"),
          FText::FromString(
              FString("Remove property instance from ACesium3DTileset")),
          this->_pFeaturesMetadataComponent.Get());
      this->_pFeaturesMetadataComponent->PreEditChange(NULL);

      propertyTable.Properties.RemoveAt(propertyIndex);

      if (propertyTable.Properties.IsEmpty()) {
        propertyTables.RemoveAt(tableIndex);
      }

      this->_pFeaturesMetadataComponent->PostEditChange();
      UKismetSystemLibrary::EndTransaction();
    }
  } else {
    TArray<FCesiumPropertyTextureDescription>& propertyTextures =
        description.ModelMetadata.PropertyTextures;
    int32 textureIndex = INDEX_NONE;
    for (int32 i = 0; i < propertyTextures.Num(); i++) {
      if (propertyTextures[i].Name == *pItem->pSourceName) {
        textureIndex = i;
        break;
      }
    }
    if (textureIndex == INDEX_NONE) {
      return;
    }
    FCesiumPropertyTextureDescription& propertyTexture =
        propertyTextures[textureIndex];

    int32 propertyIndex = INDEX_NONE;
    for (int32 i = 0; i < propertyTexture.Properties.Num(); i++) {
      if (propertyTexture.Properties[i].Name == *pItem->pPropertyId) {
        propertyIndex = i;
        break;
      }
    }

    if (propertyIndex != INDEX_NONE) {
      UKismetSystemLibrary::BeginTransaction(
          TEXT("Cesium Features / Metadata Viewer"),
          FText::FromString(
              FString("Remove property instance from ACesium3DTileset")),
          this->_pFeaturesMetadataComponent.Get());
      this->_pFeaturesMetadataComponent->PreEditChange(NULL);

      propertyTexture.Properties.RemoveAt(propertyIndex);

      if (propertyTexture.Properties.IsEmpty()) {
        propertyTextures.RemoveAt(textureIndex);
        if (this->_propertyTextureNames.Contains(*pItem->pSourceName)) {
          description.PrimitiveMetadata.PropertyTextureNames.Remove(
              *pItem->pSourceName);
        }
      }

      this->_pFeaturesMetadataComponent->PostEditChange();
      UKismetSystemLibrary::EndTransaction();
    }
  }
}

void CesiumFeaturesMetadataViewer::removeFeatureIdSetInstance(
    TSharedRef<FeatureIdSetInstance> pItem) {
  if (!this->_pFeaturesMetadataComponent.IsValid()) {
    UE_LOG(
        LogCesiumEditor,
        Error,
        TEXT(
            "This window was opened for a now invalid CesiumFeaturesMetadataComponent."))
    return;
  }

  TArray<FCesiumFeatureIdSetDescription>& featureIdSets =
      this->_pFeaturesMetadataComponent->Description.PrimitiveFeatures
          .FeatureIdSets;
  int32 featureIdSetIndex = INDEX_NONE;
  for (int32 i = 0; i < featureIdSets.Num(); i++) {
    if (featureIdSets[i].Name == *pItem->pFeatureIdSetName) {
      featureIdSetIndex = i;
      break;
    }
  }

  if (featureIdSetIndex != INDEX_NONE) {
    UKismetSystemLibrary::BeginTransaction(
        TEXT("Cesium Features / Metadata Viewer"),
        FText::FromString(
            FString("Register feature ID set instance with ACesium3DTileset")),
        this->_pFeaturesMetadataComponent.Get());
    this->_pFeaturesMetadataComponent->PreEditChange(NULL);

    featureIdSets.RemoveAt(featureIdSetIndex);

    this->_pFeaturesMetadataComponent->PostEditChange();
    UKismetSystemLibrary::EndTransaction();
  }
}

TSharedRef<FString>
CesiumFeaturesMetadataViewer::getSharedRef(const FString& string) {
  return _stringMap.Contains(string)
             ? _stringMap[string]
             : _stringMap.Emplace(string, MakeShared<FString>(string));
}

void CesiumFeaturesMetadataViewer::initializeStaticVariables() {
  if (_conversionOptions.IsEmpty()) {
    populateEnumOptions<ECesiumEncodedMetadataConversion>(_conversionOptions);
  }
  if (_encodedTypeOptions.IsEmpty()) {
    populateEnumOptions<ECesiumEncodedMetadataType>(_encodedTypeOptions);
  }
  if (_encodedComponentTypeOptions.IsEmpty()) {
    populateEnumOptions<ECesiumEncodedMetadataComponentType>(
        _encodedComponentTypeOptions);
  }
}
