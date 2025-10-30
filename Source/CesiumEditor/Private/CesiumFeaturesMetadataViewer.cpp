// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumFeaturesMetadataViewer.h"

#include "Cesium3DTileset.h"
#include "CesiumCommon.h"
#include "CesiumEditor.h"
#include "CesiumFeaturesMetadataComponent.h"
#include "CesiumModelMetadata.h"
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
                                    "Add this property to the tileset's CesiumFeaturesMetadataComponent")),
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

  FString sourceString;
  switch (pItem->source) {
  case EPropertySource::PropertyTable:
    sourceString = TEXT("Property Table");
    break;
  case EPropertySource::PropertyTexture:
    sourceString = TEXT("Property Texture");
    break;
  }

  sourceString += FString::Printf(TEXT(" (%s)"), **pItem->pSourceName);

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

  return SNew(STableRow<TSharedRef<StatisticView>>, list)
      .Content()
          [SNew(SBox)
               .HAlign(EHorizontalAlignment::HAlign_Fill)
               .Content()
                   [SNew(SHorizontalBox) +
                    SHorizontalBox::Slot().FillWidth(0.4f).Padding(5.0f).VAlign(
                        EVerticalAlignment::VAlign_Center)
                        [SNew(STextBlock)
                             .AutoWrapText(true)
                             .Text(FText::FromString(typeString))] +
                    SHorizontalBox::Slot().FillWidth(1.0f).Padding(5.0f).VAlign(
                        EVerticalAlignment::VAlign_Center)
                        [SNew(STextBlock)
                             .AutoWrapText(true)
                             .Text(FText::FromString(sourceString))] +
                    SHorizontalBox::Slot().AutoWidth().Padding(5.0f).VAlign(
                        EVerticalAlignment::VAlign_Center)
                        [SNew(STextBlock)
                             .AutoWrapText(true)
                             .Text(FText::FromString(qualifierString))] +
                    SHorizontalBox::Slot()
                        .AutoWidth()
                        .HAlign(EHorizontalAlignment::HAlign_Right)
                        .VAlign(EVerticalAlignment::VAlign_Center)
                            [PropertyCustomizationHelpers::MakeNewBlueprintButton(
                                FSimpleDelegate::CreateLambda([this, pItem]() {
                                  this->registerPropertyInstance(pItem);
                                }),
                                FText::FromString(TEXT(
                                    "Add this property to the tileset's CesiumFeaturesMetadataComponent")),
                                TAttribute<bool>::Create([this, pItem]() {
                                  return this->canBeRegistered(pItem);
                                }))]]];
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

void CesiumFeaturesMetadataViewer::Construct(const FArguments& InArgs) {
  TSharedRef<SVerticalBox> pVertical = SNew(SVerticalBox);
  this->_pContent = pVertical.ToSharedPtr();

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
                        .Padding(FMargin(10.0f))[pVertical]]);
}

void CesiumFeaturesMetadataViewer::Sync() {
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

  // This assumes that the property tables are the same across all models in
  // the tileset, and that they all have the same schema.
  for (const UActorComponent* pComponent : tileset.GetComponents()) {
    const auto* pPrimitive = Cast<UPrimitiveComponent>(pComponent);
    if (!pPrimitive) {
      continue;
    }

    FCesiumModelMetadata modelMetadata =
        UCesiumModelMetadataBlueprintLibrary::GetModelMetadata(pPrimitive);

    this->gatherGltfPropertyTables(modelMetadata);
  }
}
bool CesiumFeaturesMetadataViewer::PropertyInstanceView::operator==(
    const PropertyInstanceView& property) const {
  return *pPropertyId == *property.pPropertyId &&
         propertyDetails == property.propertyDetails &&
         source == property.source && *pSourceName == *property.pSourceName;
}

bool CesiumFeaturesMetadataViewer::PropertyInstanceView::operator!=(
    const PropertyInstanceView& property) const {
  return !operator==(property);
}

namespace {
// Copy of the function in EncodedFeaturesMetadata.h. Unfortunately the file
// is too entangled in Private code to pull into Public.
FString getNameForPropertyTable(const FCesiumPropertyTable& PropertyTable) {
  FString propertyTableName =
      UCesiumPropertyTableBlueprintLibrary::GetPropertyTableName(PropertyTable);

  if (propertyTableName.IsEmpty()) {
    // Substitute the name with the property table's class.
    propertyTableName = PropertyTable.getClassName();
  }

  return propertyTableName;
}
} // namespace

void CesiumFeaturesMetadataViewer::gatherGltfPropertyTables(
    const FCesiumModelMetadata& modelMetadata) {
  const TArray<FCesiumPropertyTable>& propertyTables =
      UCesiumModelMetadataBlueprintLibrary::GetPropertyTables(modelMetadata);

  for (const FCesiumPropertyTable& propertyTable : propertyTables) {
    FString propertyTableName = getNameForPropertyTable(propertyTable);

    const TMap<FString, FCesiumPropertyTableProperty>& properties =
        UCesiumPropertyTableBlueprintLibrary::GetProperties(propertyTable);
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
          UCesiumPropertyTablePropertyBlueprintLibrary::GetValueType(
              propertyIt.Value);
      propertyDetails.Type = valueType.Type;
      propertyDetails.ComponentType = valueType.ComponentType;
      propertyDetails.bIsArray = valueType.bIsArray;
      propertyDetails.ArraySize =
          UCesiumPropertyTablePropertyBlueprintLibrary::GetArraySize(
              propertyIt.Value);
      propertyDetails.bIsNormalized =
          UCesiumPropertyTablePropertyBlueprintLibrary::IsNormalized(
              propertyIt.Value);

      FCesiumMetadataValue offset =
          UCesiumPropertyTablePropertyBlueprintLibrary::GetOffset(
              propertyIt.Value);
      propertyDetails.bHasOffset =
          !UCesiumMetadataValueBlueprintLibrary::IsEmpty(offset);

      FCesiumMetadataValue scale =
          UCesiumPropertyTablePropertyBlueprintLibrary::GetScale(
              propertyIt.Value);
      propertyDetails.bHasScale =
          !UCesiumMetadataValueBlueprintLibrary::IsEmpty(scale);

      FCesiumMetadataValue noData =
          UCesiumPropertyTablePropertyBlueprintLibrary::GetNoDataValue(
              propertyIt.Value);
      propertyDetails.bHasNoDataValue =
          !UCesiumMetadataValueBlueprintLibrary::IsEmpty(scale);

      FCesiumMetadataValue defaultValue =
          UCesiumPropertyTablePropertyBlueprintLibrary::GetDefaultValue(
              propertyIt.Value);
      propertyDetails.bHasDefaultValue =
          !UCesiumMetadataValueBlueprintLibrary::IsEmpty(scale);

      PropertyInstanceView instance{
          .pPropertyId = MakeShared<FString>(propertyIt.Key),
          .propertyDetails = std::move(propertyDetails),
          .source = EPropertySource::PropertyTable,
          .pSourceName = MakeShared<FString>(propertyTableName)};

      bool instanceExists = false;
      for (const TSharedRef<PropertyInstanceView>& propertyInstance :
           property.instances) {
        if (instance == *propertyInstance) {
          instanceExists = true;
          break;
        }
      }

      if (!instanceExists) {
        property.instances.Emplace(
            MakeShared<PropertyInstanceView>(std::move(instance)));
      }
    }
  }
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

  FCesiumMetadataClassStatisticsDescription* pClass =
      statistics.FindByPredicate(
          [classId = *pItem->pClassId](
              const FCesiumMetadataClassStatisticsDescription& existingClass) {
            return existingClass.Id == classId;
          });

  if (!pClass) {
    return true;
  }

  FCesiumMetadataPropertyStatisticsDescription* pProperty =
      pClass->Properties.FindByPredicate(
          [propertyId = *pItem->pPropertyId](
              const FCesiumMetadataPropertyStatisticsDescription&
                  existingProperty) {
            return existingProperty.Id == propertyId;
          });

  if (!pProperty) {
    return true;
  }

  return pProperty->Values.FindByPredicate(
             [semantic = pItem->semantic](
                 const FCesiumMetadataPropertyStatisticValue& existingValue) {
               return existingValue.Semantic == semantic;
             }) == nullptr;
}

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

  if (pItem->source == EPropertySource::PropertyTable) {
    FCesiumPropertyTablePropertyDescription* pProperty = findProperty<
        FCesiumPropertyTableDescription,
        FCesiumPropertyTablePropertyDescription>(
        featuresMetadata.Description.ModelMetadata.PropertyTables,
        *pItem->pSourceName,
        *pItem->pPropertyId,
        false);
    if (!pProperty)
      return true;

    return pProperty->PropertyDetails != pItem->propertyDetails;
  }

  if (pItem->source == EPropertySource::PropertyTexture) {
    FCesiumPropertyTexturePropertyDescription* pProperty = findProperty<
        FCesiumPropertyTextureDescription,
        FCesiumPropertyTexturePropertyDescription>(
        featuresMetadata.Description.ModelMetadata.PropertyTextures,
        *pItem->pSourceName,
        *pItem->pPropertyId,
        false);
    if (!pProperty)
      return true;

    return pProperty->PropertyDetails != pItem->propertyDetails;
  }

  return false;
}

void CesiumFeaturesMetadataViewer::registerStatistic(
    TSharedRef<StatisticView> pItem) {
  if (!this->_pFeaturesMetadataComponent.IsValid()) {
    return;
  }

  UCesiumFeaturesMetadataComponent& featuresMetadata =
      *this->_pFeaturesMetadataComponent;

  TArray<FCesiumMetadataClassStatisticsDescription>& statistics =
      featuresMetadata.Description.Statistics;

  FCesiumMetadataClassStatisticsDescription* pClass =
      statistics.FindByPredicate(
          [classId = *pItem->pClassId](
              const FCesiumMetadataClassStatisticsDescription& existingClass) {
            return existingClass.Id == classId;
          });

  if (!pClass) {
    int32 index = statistics.Emplace(
        *pItem->pClassId,
        TArray<FCesiumMetadataPropertyStatisticsDescription>());
    pClass = &statistics[index];
  }

  FCesiumMetadataPropertyStatisticsDescription* pProperty =
      pClass->Properties.FindByPredicate(
          [propertyId = *pItem->pPropertyId](
              const FCesiumMetadataPropertyStatisticsDescription&
                  existingProperty) {
            return existingProperty.Id == propertyId;
          });

  if (!pProperty) {
    int32 index = pClass->Properties.Emplace(*pItem->pPropertyId);
    pProperty = &pClass->Properties[index];
  }
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
}

void CesiumFeaturesMetadataViewer::registerPropertyInstance(
    TSharedRef<PropertyInstanceView> pItem) {
  if (!this->_pTileset.IsValid() ||
      !this->_pFeaturesMetadataComponent.IsValid()) {
    UE_LOG(
        LogCesiumEditor,
        Error,
        TEXT(
            "This window was opened for a now invalid CesiumFeaturesMetadataComponent."))
    return;
  }

  ACesium3DTileset& tileset = *this->_pTileset;
  UCesiumFeaturesMetadataComponent& featuresMetadata =
      *this->_pFeaturesMetadataComponent;

  if (pItem->source == EPropertySource::PropertyTable) {
    TArray<FCesiumPropertyTableDescription>& propertyTables =
        featuresMetadata.Description.ModelMetadata.PropertyTables;
    FCesiumPropertyTableDescription* pPropertyTable =
        propertyTables.FindByPredicate(
            [&name = *pItem->pSourceName](
                const FCesiumPropertyTableDescription& existingTable) {
              return name == existingTable.Name;
            });

    if (!pPropertyTable) {
      int32 index = propertyTables.Emplace(
          *pItem->pSourceName,
          TArray<FCesiumPropertyTablePropertyDescription>());
      pPropertyTable = &propertyTables[index];
    }

    FCesiumPropertyTablePropertyDescription* pProperty =
        pPropertyTable->Properties.FindByPredicate(
            [&name = *pItem->pPropertyId](
                const FCesiumPropertyTablePropertyDescription&
                    existingProperty) {
              return name == existingProperty.Name;
            });

    if (!pProperty) {
      int32 index = pPropertyTable->Properties.Emplace();
      pProperty = &pPropertyTable->Properties[index];
      pProperty->Name = *pItem->pPropertyId;
    }

    FCesiumPropertyTablePropertyDescription& property = *pProperty;
    property.PropertyDetails = pItem->propertyDetails;
  }
}
