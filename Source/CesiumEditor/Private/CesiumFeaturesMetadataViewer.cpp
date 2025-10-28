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
                                true)]]];
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
    TSharedRef<SVerticalBox>& pVertical,
    const ClassStatisticsView& classStatistics) {
  pVertical->AddSlot().AutoHeight()
      [SNew(SExpandableArea)
           .InitiallyCollapsed(true)
           .HeaderContent()[SNew(STextBlock)
                                .Text(FText::FromString(*classStatistics.pId))]
           .BodyContent()
               [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(0.05f) +
                SHorizontalBox::Slot()
                    [SNew(SListView<TSharedRef<PropertyStatisticsView>>)
                         .ListItemsSource(&classStatistics.properties)
                         .OnGenerateRow(
                             this,
                             &CesiumFeaturesMetadataViewer::
                                 createPropertyStatisticsDropdown)]]];
}

TSharedRef<ITableRow> CesiumFeaturesMetadataViewer::createPropertyInstanceRow(
    TSharedRef<PropertyInstanceView> pItem,
    const TSharedRef<STableViewBase>& list) {
  FString typeString = pItem->type.ToString();
  if (pItem->isNormalized) {
    typeString += " (Normalized)";
  }

  if (pItem->type.bIsArray) {
  }

  FString sourceString;
  switch (pItem->source) {
  case EPropertySource::PropertyTable:
    sourceString = "Property Table";
    break;
  case EPropertySource::PropertyTexture:
    sourceString = "Property Texture";
    break;
  }

  sourceString += " (" + *pItem->sourceName + ")";

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
                             .Text(FText::FromString(typeString))] +
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
                                  this->registerPropertyInstance(pItem);
                                }),
                                FText::FromString(TEXT(
                                    "Add this property to the tileset's CesiumFeaturesMetadataComponent")),
                                true)]]];
}

void CesiumFeaturesMetadataViewer::createGltfPropertyDropdown(
    TSharedRef<SVerticalBox>& pVertical,
    const PropertyView& property) {
  pVertical->AddSlot().AutoHeight()
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
              600,
              800))[SNew(SBorder)
                        .Visibility(EVisibility::Visible)
                        .BorderImage(FAppStyle::GetBrush("Menu.Background"))
                        .Padding(FMargin(10.0f))[pVertical]]);
}

void CesiumFeaturesMetadataViewer::Sync() {
  this->_statisticsClasses.Empty();
  //  this->_classes.Empty();

  this->gatherTilesetStatistics();
  this->gatherGltfMetadata();

  TSharedRef<SVerticalBox> pContent = this->_pContent.ToSharedRef();
  pContent->ClearChildren();

  pContent->AddSlot().AutoHeight()
      [SNew(SHeader)
           .Content()[SNew(STextBlock)
                          .TextStyle(FCesiumEditorModule::GetStyle(), "Heading")
                          .Text(FText::FromString("Tileset Statistics"))
                          .Margin(FMargin(0.f, 10.f))]];

  if (this->_statisticsClasses.IsEmpty()) {
    pContent->AddSlot().AutoHeight()
        [SNew(STextBlock)
             .AutoWrapText(true)
             .Margin(FMargin(0.0f, 15.0f))
             .Text(FText::FromString(TEXT(
                 "This tileset does not contain any pre-computed statistics.")))];
  } else {
    for (const ClassStatisticsView& theClass : this->_statisticsClasses) {
      this->createClassStatisticsDropdown(pContent, theClass);
    }
  }

  pContent->AddSlot().AutoHeight()
      [SNew(SHeader)
           .Content()[SNew(STextBlock)
                          .TextStyle(FCesiumEditorModule::GetStyle(), "Heading")
                          .Text(FText::FromString("glTF Metadata"))
                          .Margin(FMargin(0.f, 10.f))]];

  if (this->_metadataProperties.IsEmpty()) {
    pContent->AddSlot().AutoHeight()
        [SNew(STextBlock)
             .AutoWrapText(true)
             .Margin(FMargin(0.0f, 15.0f))
             .Text(FText::FromString(
                 TEXT("This tileset does not contain any glTF metadata.")))];
  } else {
    for (const PropertyView& property : this->_metadataProperties) {
      this->createGltfPropertyDropdown(pContent, property);
    }
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

    // AutoFillPropertyTextureDescriptions(
    //     this->Description.ModelMetadata.PropertyTextures,
    //     modelMetadata);

    // TArray<USceneComponent*> childComponents;
    // pGltf->GetChildrenComponents(false, childComponents);

    // for (const USceneComponent* pChildComponent : childComponents) {
    //   const auto* pCesiumPrimitive =
    //   Cast<ICesiumPrimitive>(pChildComponent); if (!pCesiumPrimitive) {
    //     continue;
    //   }
    //   const CesiumPrimitiveData& primData =
    //       pCesiumPrimitive->getPrimitiveData();
    //   const FCesiumPrimitiveFeatures& primitiveFeatures =
    //   primData.Features; const TArray<FCesiumPropertyTable>& propertyTables
    //   =
    //       UCesiumModelMetadataBlueprintLibrary::GetPropertyTables(
    //           modelMetadata);
    //   const FCesiumPrimitiveFeatures* pInstanceFeatures = nullptr;
    //   const auto* pInstancedComponent =
    //       Cast<UCesiumGltfInstancedComponent>(pChildComponent);
    //   if (pInstancedComponent) {
    //     pInstanceFeatures = pInstancedComponent->pInstanceFeatures.Get();
    //   }
    //   AutoFillFeatureIdSetDescriptions(
    //       this->Description.PrimitiveFeatures.FeatureIdSets,
    //       primitiveFeatures,
    //       pInstanceFeatures,
    //       propertyTables);

    //  const FCesiumPrimitiveMetadata& primitiveMetadata = primData.Metadata;
    //  const TArray<FCesiumPropertyTexture>& propertyTextures =
    //      UCesiumModelMetadataBlueprintLibrary::GetPropertyTextures(
    //          modelMetadata);
    //  AutoFillPropertyTextureNames(
    //      this->Description.PrimitiveMetadata.PropertyTextureNames,
    //      primitiveMetadata,
    //      propertyTextures);
    //}
  }
}

bool CesiumFeaturesMetadataViewer::PropertyInstanceView::operator==(
    const PropertyInstanceView& property) const {
  return *pPropertyId == *property.pPropertyId && type == property.type &&
         arraySize == property.arraySize &&
         isNormalized == property.isNormalized && source == property.source &&
         *sourceName == *property.sourceName &&
         qualifiers == property.qualifiers;
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

      const FCesiumMetadataValueType valueType =
          UCesiumPropertyTablePropertyBlueprintLibrary::GetValueType(
              propertyIt.Value);
      int64 arraySize =
          UCesiumPropertyTablePropertyBlueprintLibrary::GetArraySize(
              propertyIt.Value);
      bool isNormalized =
          UCesiumPropertyTablePropertyBlueprintLibrary::IsNormalized(
              propertyIt.Value);

      uint8 propertyQualifiers = 0;

      FCesiumMetadataValue offset =
          UCesiumPropertyTablePropertyBlueprintLibrary::GetOffset(
              propertyIt.Value);
      propertyQualifiers |=
          uint8(!UCesiumMetadataValueBlueprintLibrary::IsEmpty(offset))
          << uint8(EPropertyQualifiers::Offset);

      FCesiumMetadataValue scale =
          UCesiumPropertyTablePropertyBlueprintLibrary::GetOffset(
              propertyIt.Value);
      propertyQualifiers |=
          uint8(!UCesiumMetadataValueBlueprintLibrary::IsEmpty(scale))
          << uint8(EPropertyQualifiers::Scale);

      FCesiumMetadataValue noData =
          UCesiumPropertyTablePropertyBlueprintLibrary::GetNoDataValue(
              propertyIt.Value);
      propertyQualifiers |=
          uint8(!UCesiumMetadataValueBlueprintLibrary::IsEmpty(scale))
          << uint8(EPropertyQualifiers::NoData);

      FCesiumMetadataValue defaultValue =
          UCesiumPropertyTablePropertyBlueprintLibrary::GetDefaultValue(
              propertyIt.Value);
      propertyQualifiers |=
          uint8(!UCesiumMetadataValueBlueprintLibrary::IsEmpty(scale))
          << uint8(EPropertyQualifiers::Default);

      PropertyInstanceView instance{
          .pPropertyId = MakeShared<FString>(propertyIt.Key),
          .type = valueType,
          .arraySize = arraySize,
          .isNormalized = isNormalized,
          .source = EPropertySource::PropertyTable,
          .sourceName = MakeShared<FString>(propertyTableName),
          .qualifiers = propertyQualifiers};

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

void CesiumFeaturesMetadataViewer::registerStatistic(
    TSharedRef<StatisticView> pItem) {
  if (!this->_pTileset.IsValid()) {
    return;
  }

  if (!this->_pFeaturesMetadataComponent.IsValid()) {
    return;
  }

  ACesium3DTileset& tileset = *this->_pTileset;
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
  if (!this->_pTileset.IsValid()) {
    return;
  }

  if (!this->_pFeaturesMetadataComponent.IsValid()) {
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
            [&name = *pItem->sourceName](
                const FCesiumPropertyTableDescription& existingTable) {
              return name == existingTable.Name;
            });

    if (!pPropertyTable) {
      int32 index = propertyTables.Emplace(
          *pItem->sourceName,
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
    property.PropertyDetails.Type = pItem->type.Type;
    property.PropertyDetails.ComponentType = pItem->type.ComponentType;
    property.PropertyDetails.bIsArray = pItem->type.bIsArray;
    property.PropertyDetails.ArraySize = pItem->arraySize;
    property.PropertyDetails.bIsNormalized = pItem->isNormalized;
    property.PropertyDetails.bHasOffset =
        pItem->qualifiers & 1u << uint8(EPropertyQualifiers::Offset);
    property.PropertyDetails.bHasScale =
        pItem->qualifiers & 1u << uint8(EPropertyQualifiers::Scale);
    property.PropertyDetails.bHasNoDataValue =
        pItem->qualifiers & 1u << uint8(EPropertyQualifiers::NoData);
    property.PropertyDetails.bHasDefaultValue =
        pItem->qualifiers & 1u << uint8(EPropertyQualifiers::Default);
  }
}
