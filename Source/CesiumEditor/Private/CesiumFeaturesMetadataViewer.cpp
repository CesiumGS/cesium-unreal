// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumFeaturesMetadataViewer.h"

#include "Cesium3DTileset.h"
#include "CesiumCommon.h"
#include "CesiumEditor.h"
#include "CesiumFeaturesMetadataComponent.h"
#include "CesiumMetadataEncodingDetails.h"
#include "CesiumModelMetadata.h"
#include "CesiumPrimitiveFeatures.h"
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

  this->_statisticsClasses.Empty();
  this->_metadataProperties.Empty();

  this->gatherTilesetStatistics();
  this->gatherGltfMetadata();

  TSharedRef<SVerticalBox> pContent = this->_pContent.ToSharedRef();
  pContent->ClearChildren();

  pContent->AddSlot().AutoHeight()
      [SNew(SHeader)
           .Content()[SNew(STextBlock)
                          .TextStyle(FCesiumEditorModule::GetStyle(), "Heading")
                          .Text(FText::FromString(TEXT("Tileset Statistics")))
                          .Margin(FMargin(0.f, 10.f))]];

  if (this->_statisticsClasses.IsEmpty()) {
    pContent->AddSlot().AutoHeight()
        [SNew(STextBlock)
             .AutoWrapText(true)
             .Text(FText::FromString(TEXT(
                 "This tileset does not contain any pre-computed statistics.")))];
  } else {
    TSharedRef<SScrollBox> pStatisticsContent = SNew(SScrollBox);
    for (const ClassStatisticsView& theClass : this->_statisticsClasses) {
      this->createClassStatisticsDropdown(pStatisticsContent, theClass);
    }
    pContent->AddSlot().AutoHeight()[pStatisticsContent];
  }

  pContent->AddSlot().AutoHeight()
      [SNew(SHeader)
           .Content()[SNew(STextBlock)
                          .TextStyle(FCesiumEditorModule::GetStyle(), "Heading")
                          .Text(FText::FromString(TEXT("glTF Metadata")))
                          .Margin(FMargin(0.f, 10.f))]];

  if (this->_metadataProperties.IsEmpty()) {
    pContent->AddSlot().AutoHeight()
        [SNew(STextBlock)
             .AutoWrapText(true)
             .Text(FText::FromString(
                 TEXT("This tileset does not contain any glTF metadata.")))];
  } else {
    TSharedRef<SScrollBox> pGltfContent = SNew(SScrollBox);
    for (const PropertyView& property : this->_metadataProperties) {
      this->createGltfPropertyDropdown(pGltfContent, property);
    }
    pContent->AddSlot().FillHeight(1.0)[pGltfContent];
  }
}

void CesiumFeaturesMetadataViewer::gatherTilesetStatistics() {
  if (!this->_pTileset.IsValid())
    return;

  ACesium3DTileset& tileset = *this->_pTileset;
  const Cesium3DTilesSelection::TilesetMetadata* pMetadata =
      tileset.GetTileset() ? tileset.GetTileset()->getMetadata() : nullptr;

  const std::optional<Cesium3DTiles::Statistics>& maybeStatistics =
      pMetadata ? pMetadata->statistics : std::nullopt;
  if (!maybeStatistics) {
    return;
  }

  if (!pMetadata->schema) {
    UE_LOG(
        LogCesiumEditor,
        Warning,
        TEXT(
            "Tileset {} contains statistics but has no metadata schema to qualify them."),
        *tileset.GetActorLabel());
    return;
  }
  const Cesium3DTiles::Schema& schema = *pMetadata->schema;

  for (const auto classIt : maybeStatistics->classes) {
    if (schema.classes.find(classIt.first) == schema.classes.end()) {
      UE_LOG(
          LogCesiumEditor,
          Warning,
          TEXT(
              "Tileset {} contains statistics for class {}, but it is missing from the metadata schema."),
          *tileset.GetActorLabel());
      continue;
    }

    const Cesium3DTiles::Class& tilesetClass = schema.classes.at(classIt.first);

    TSharedRef<FString> pClassId = MakeShared<FString>(classIt.first.c_str());
    TArray<TSharedRef<PropertyStatisticsView>> properties;

    for (const auto propertyIt : classIt.second.properties) {
      if (tilesetClass.properties.find(propertyIt.first) ==
          tilesetClass.properties.end()) {
        UE_LOG(
            LogCesiumEditor,
            Warning,
            TEXT(
                "Tileset {} contains statistics for property {} in class {}, but it is missing from the schema."),
            *tileset.GetActorLabel());
        continue;
      }

      FCesiumMetadataValueType valueType =
          FCesiumMetadataValueType::fromClassProperty(
              tilesetClass.properties.at(propertyIt.first));

      // Statistics are only applicable to numeric types, so skip for all
      // others.
      switch (valueType.Type) {
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

      TSharedRef<FString> pPropertyId =
          MakeShared<FString>(propertyIt.first.c_str());
      TSharedRef<PropertyStatisticsView> pProperty =
          MakeShared<PropertyStatisticsView>(
              pClassId,
              pPropertyId,
              TArray<TSharedRef<StatisticView>>());

      if (propertyIt.second.min) {
        TSharedRef<StatisticView> statistic = MakeShared<StatisticView>(
            pClassId,
            pPropertyId,
            ECesiumMetadataStatisticSemantic::Min,
            FCesiumMetadataValue::fromJsonValue(
                valueType,
                *propertyIt.second.min));
        pProperty->statistics.Emplace(std::move(statistic));
      }

      if (propertyIt.second.max) {
        TSharedRef<StatisticView> statistic = MakeShared<StatisticView>(
            pClassId,
            pPropertyId,
            ECesiumMetadataStatisticSemantic::Max,
            FCesiumMetadataValue::fromJsonValue(
                valueType,
                *propertyIt.second.max));
        pProperty->statistics.Emplace(std::move(statistic));
      }

      properties.Add(std::move(pProperty));
    }

    ClassStatisticsView classStatistics{pClassId, std::move(properties)};
    this->_statisticsClasses.Emplace(std::move(classStatistics));
  }
}

void CesiumFeaturesMetadataViewer::gatherGltfMetadata() {
  if (!this->_pTileset.IsValid()) {
    return;
  }

  ACesium3DTileset& tileset = *this->_pTileset;

  for (const UActorComponent* pComponent : tileset.GetComponents()) {
    const auto* pPrimitive = Cast<UPrimitiveComponent>(pComponent);
    if (!pPrimitive) {
      continue;
    }

    FCesiumModelMetadata modelMetadata =
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
  }
}

void CesiumFeaturesMetadataViewer::gatherGltfFeatures() {
  if (!this->_pTileset.IsValid()) {
    return;
  }

  ACesium3DTileset& tileset = *this->_pTileset;

  for (const UActorComponent* pComponent : tileset.GetComponents()) {
    const auto* pPrimitive = Cast<UPrimitiveComponent>(pComponent);
    if (!pPrimitive) {
      continue;
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
    }
  }
}

bool CesiumFeaturesMetadataViewer::PropertyInstanceView::operator==(
    const PropertyInstanceView& rhs) const {
  if (*pPropertyId != *rhs.pPropertyId ||
      propertyDetails != rhs.propertyDetails ||
      *pSourceName != *rhs.pSourceName) {
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

bool CesiumFeaturesMetadataViewer::PropertyInstanceView::operator!=(
    const PropertyInstanceView& property) const {
  return !operator==(property);
}

namespace {
// These are copies of fynctions in EncodedFeaturesMetadata.h. The file is
// unfortunately too entangled in Private code to pull into Public.
FString getNameForPropertySource(const FCesiumPropertyTable& PropertyTable) {
  FString propertyTableName =
      UCesiumPropertyTableBlueprintLibrary::GetPropertyTableName(PropertyTable);

  if (propertyTableName.IsEmpty()) {
    // Substitute the name with the property table's class.
    propertyTableName = PropertyTable.getClassName();
  }

  return propertyTableName;
}

FString
getNameForPropertySource(const FCesiumPropertyTexture& PropertyTexture) {
  FString propertyTextureName =
      UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureName(
          PropertyTexture);

  if (propertyTextureName.IsEmpty()) {
    // Substitute the name with the property texture's class.
    propertyTextureName = PropertyTexture.getClassName();
  }

  return propertyTextureName;
}

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
    FString propertySourceName = getNameForPropertySource(source);

    const TMap<FString, TSourceProperty>& properties =
        TSourceBlueprintLibrary::GetProperties(source);
    for (const auto& propertyIt : properties) {
      PropertyView* pProperty = this->_metadataProperties.FindByPredicate(
          [&propertyId = propertyIt.Key](const PropertyView& existingProperty) {
            return *existingProperty.pId == propertyId;
          });

      if (!pProperty) {
        int32 index = this->_metadataProperties.Emplace(
            MakeShared<FString>(propertyIt.Key),
            TArray<TSharedRef<PropertyInstanceView>>());
        pProperty = &this->_metadataProperties[index];
      }

      PropertyView& property = *pProperty;

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

      PropertyInstanceView instance{
          .pPropertyId = MakeShared<FString>(propertyIt.Key),
          .propertyDetails = std::move(propertyDetails),
          .pSourceName = MakeShared<FString>(propertySourceName)};

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

      TSharedRef<PropertyInstanceView>* pExistingInstance =
          property.instances.FindByPredicate(
              [&instance](
                  const TSharedRef<PropertyInstanceView>& existingInstance) {
                return instance == *existingInstance;
              });
      if (!pExistingInstance) {
        property.instances.Emplace(
            MakeShared<PropertyInstanceView>(std::move(instance)));
      }
    }
  }
}

namespace {
FString statisticSemanticToString(ECesiumMetadataStatisticSemantic semantic) {
  const UEnum* pEnum = StaticEnum<ECesiumMetadataStatisticSemantic>();
  return pEnum ? pEnum->GetNameStringByValue((int64)semantic) : FString();
}
} // namespace

TSharedRef<ITableRow> CesiumFeaturesMetadataViewer::createStatisticRow(
    TSharedRef<StatisticView> pItem,
    const TSharedRef<STableViewBase>& list) {
  return SNew(STableRow<TSharedRef<StatisticView>>, list)
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
                                 statisticSemanticToString(pItem->semantic)))] +
                    SHorizontalBox::Slot().FillWidth(1.0f).Padding(5.0f).VAlign(
                        EVerticalAlignment::VAlign_Center)
                        [SNew(STextBlock)
                             .AutoWrapText(true)
                             .Text(FText::FromString(
                                 UCesiumMetadataValueBlueprintLibrary::
                                     GetString(pItem->value, FString())))] +
                    SHorizontalBox::Slot()
                        .AutoWidth()
                        .HAlign(EHorizontalAlignment::HAlign_Right)
                        .VAlign(EVerticalAlignment::VAlign_Center)
                            [PropertyCustomizationHelpers::MakeNewBlueprintButton(
                                FSimpleDelegate::CreateLambda([this, pItem]() {
                                  this->registerStatistic(pItem);
                                }),
                                FText::FromString(TEXT(
                                    "Add this property statistic to the tileset's CesiumFeaturesMetadataComponent.")),
                                TAttribute<bool>::Create([this, pItem]() {
                                  return this->canBeRegistered(pItem);
                                }))]]];
}

TSharedRef<ITableRow>
CesiumFeaturesMetadataViewer::createPropertyStatisticsDropdown(
    TSharedRef<PropertyStatisticsView> pItem,
    const TSharedRef<STableViewBase>& list) {
  return SNew(STableRow<TSharedRef<PropertyStatisticsView>>, list)
      .Content()[SNew(SExpandableArea)
                     .InitiallyCollapsed(true)
                     .HeaderContent()[SNew(STextBlock)
                                          .Text(FText::FromString(*pItem->pId))]
                     .BodyContent()[SNew(SListView<TSharedRef<StatisticView>>)
                                        .ListItemsSource(&pItem->statistics)
                                        .SelectionMode(ESelectionMode::None)
                                        .OnGenerateRow(
                                            this,
                                            &CesiumFeaturesMetadataViewer::
                                                createStatisticRow)]];
}

void CesiumFeaturesMetadataViewer::createClassStatisticsDropdown(
    TSharedRef<SScrollBox>& pContent,
    const ClassStatisticsView& classStatistics) {

  pContent->AddSlot()
      [SNew(SExpandableArea)
           .InitiallyCollapsed(true)
           .HeaderContent()[SNew(STextBlock)
                                .Text(FText::FromString(*classStatistics.pId))]
           .BodyContent()
               [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(0.05f) +
                SHorizontalBox::Slot()
                    [SNew(SListView<TSharedRef<PropertyStatisticsView>>)
                         .ListItemsSource(&classStatistics.properties)
                         .SelectionMode(ESelectionMode::None)
                         .OnGenerateRow(
                             this,
                             &CesiumFeaturesMetadataViewer::
                                 createPropertyStatisticsDropdown)]]];
}

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
    TSharedRef<PropertyInstanceView> pItem,
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

  FString sourceString;
  if (pTableSourceDetails) {
    sourceString = TEXT("Property Table");
  } else if (pTextureSourceDetails) {
    sourceString = TEXT("Property Texture");
  }
  sourceString += FString::Printf(TEXT(" (\"%s\")"), **pItem->pSourceName);

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
      SHorizontalBox::Slot().FillWidth(0.75f).Padding(5.0f).VAlign(
          EVerticalAlignment::VAlign_Center)
          [SNew(STextBlock)
               .AutoWrapText(true)
               .Text(FText::FromString(sourceString))
               .ToolTipText(FText::FromString(
                   "The source of the property's data. Property attributes are currently unsupported."))] +
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
        TSharedPtr<ECesiumEncodedMetadataConversion> pSelected =
            pTableSourceDetails->pConversionCombo->GetSelectedItem();
        show = pSelected.IsValid() &&
               *pSelected == ECesiumEncodedMetadataConversion::Coerce;
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

  return SNew(STableRow<TSharedRef<StatisticView>>, list)
      .Content()[SNew(SBox)
                     .HAlign(EHorizontalAlignment::HAlign_Fill)
                     .Content()[content]];
}

void CesiumFeaturesMetadataViewer::createGltfPropertyDropdown(
    TSharedRef<SScrollBox>& pContent,
    const PropertyView& property) {
  pContent->AddSlot()
      [SNew(SExpandableArea)
           .InitiallyCollapsed(true)
           .HeaderContent()[SNew(STextBlock)
                                .Text(FText::FromString(*property.pId))]
           .BodyContent()[SNew(SListView<TSharedRef<PropertyInstanceView>>)
                              .ListItemsSource(&property.instances)
                              .SelectionMode(ESelectionMode::None)
                              .OnGenerateRow(
                                  this,
                                  &CesiumFeaturesMetadataViewer::
                                      createPropertyInstanceRow)]];
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

namespace {
FCesiumMetadataPropertyStatisticsDescription* findPropertyStatistic(
    TArray<FCesiumMetadataClassStatisticsDescription>& statistics,
    const FString& classId,
    const FString& propertyId,
    bool createIfMissing) {
  FCesiumMetadataClassStatisticsDescription* pClass =
      statistics.FindByPredicate(
          [&classId](
              const FCesiumMetadataClassStatisticsDescription& existingClass) {
            return existingClass.Id == classId;
          });
  if (!pClass && !createIfMissing) {
    return nullptr;
  }

  if (!pClass) {
    int32 index = statistics.Emplace(
        classId,
        TArray<FCesiumMetadataPropertyStatisticsDescription>());
    pClass = &statistics[index];
  }

  FCesiumMetadataPropertyStatisticsDescription* pProperty =
      pClass->Properties.FindByPredicate(
          [&propertyId](const FCesiumMetadataPropertyStatisticsDescription&
                            existingProperty) {
            return existingProperty.Id == propertyId;
          });

  if (!pProperty && createIfMissing) {
    int32 index = pClass->Properties.Emplace(propertyId);
    pProperty = &pClass->Properties[index];
  }

  return pProperty;
}

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
} // namespace

bool CesiumFeaturesMetadataViewer::canBeRegistered(
    TSharedRef<StatisticView> pItem) {
  if (!this->_pFeaturesMetadataComponent.IsValid()) {
    return false;
  }

  UCesiumFeaturesMetadataComponent& featuresMetadata =
      *this->_pFeaturesMetadataComponent;

  TArray<FCesiumMetadataClassStatisticsDescription>& statistics =
      featuresMetadata.Description.Statistics;

  FCesiumMetadataPropertyStatisticsDescription* pProperty =
      findPropertyStatistic(
          statistics,
          *pItem->pClassId,
          *pItem->pPropertyId,
          false);
  if (!pProperty) {
    return true;
  }

  return pProperty->Values.FindByPredicate(
             [semantic = pItem->semantic](
                 const FCesiumMetadataPropertyStatisticValue& existingValue) {
               return existingValue.Semantic == semantic;
             }) == nullptr;
}

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
    TSharedRef<PropertyInstanceView> pItem) {
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
      // Ensure that we're coercing to a valid type.
      if (!selectedEncodingDetails.HasValidType())
        return false;
      else
        break;
    case ECesiumEncodedMetadataConversion::ParseColorFromString:
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

void CesiumFeaturesMetadataViewer::registerStatistic(
    TSharedRef<StatisticView> pItem) {
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

  TArray<FCesiumMetadataClassStatisticsDescription>& statistics =
      featuresMetadata.Description.Statistics;

  FCesiumMetadataPropertyStatisticsDescription* pProperty =
      findPropertyStatistic(
          statistics,
          *pItem->pClassId,
          *pItem->pPropertyId,
          true);
  CESIUM_ASSERT(pProperty != nullptr);

  featuresMetadata.PreEditChange(NULL);

  if (FCesiumMetadataPropertyStatisticValue* pValue =
          pProperty->Values.FindByPredicate(
              [semantic = pItem->semantic](
                  const FCesiumMetadataPropertyStatisticValue& existingValue) {
                return existingValue.Semantic == semantic;
              });
      pValue != nullptr) {
    pValue->Value = pItem->value;
  } else {
    pProperty->Values.Emplace(pItem->semantic, pItem->value);
  }

  featuresMetadata.PostEditChange();
}

void CesiumFeaturesMetadataViewer::registerPropertyInstance(
    TSharedRef<PropertyInstanceView> pItem) {
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
