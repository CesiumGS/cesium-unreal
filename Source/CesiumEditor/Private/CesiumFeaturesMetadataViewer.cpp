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

using namespace CesiumIonClient;

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
}

TSharedRef<ITableRow> CesiumFeaturesMetadataViewer::createStatisticRow(
    TSharedRef<StatisticView> pItem,
    const TSharedRef<STableViewBase>& list) {
  return SNew(STableRow<TSharedRef<StatisticView>>, list)
      .Content()
          [SNew(SBox)
               .HAlign(EHorizontalAlignment::HAlign_Fill)
               .Content()
                   [SNew(SHorizontalBox) +
                    SHorizontalBox::Slot().AutoWidth().Padding(5.0f).VAlign(
                        EVerticalAlignment::VAlign_Center)
                        [SNew(STextBlock)
                             .AutoWrapText(true)
                             .Text(FText::FromString(pItem->id))] +
                    SHorizontalBox::Slot().AutoWidth().Padding(5.0f).VAlign(
                        EVerticalAlignment::VAlign_Center)
                        [SNew(STextBlock)
                             .AutoWrapText(true)
                             .Text(FText::FromString(
                                 UCesiumMetadataValueBlueprintLibrary::
                                     GetString(pItem->value, FString())))] +
                    SHorizontalBox::Slot().AutoWidth().VAlign(
                        EVerticalAlignment::VAlign_Center)
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
                                          .Text(FText::FromString(
                                              "Property " + *pItem->pId))]
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
                                .Text(FText::FromString(
                                    "Class " + *classStatistics.pId))]
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

// Identifiers for the columns of the various table views
// static FName ColumnName_Id = "Id";
// static FName ColumnName_Type = "Type";
// static FName ColumnName_Name = "Name";
// static FName ColumnName_Value = "Value";
// static FName ColumnName_Add = "Add";

void CesiumFeaturesMetadataViewer::createGltfClassDropdown(
    TSharedRef<SVerticalBox>& pVertical) {
  // pVertical->AddSlot().AutoHeight()
  //    [SNew(SExpandableArea)
  //         .InitiallyCollapsed(true)
  //         .BodyContent()[
  //             SNew(SListView<TSharedPtr<
  //                      FCesiumMetadataPropertyStatisticsDescription>>).ListItemsSource(&properties)
  //                 .OnMouseButtonDoubleClick(this,
  //                 &CesiumFeaturesMetadataViewer::registerStatistic)
  //                 .OnGenerateRow(this, &CesiumIonPanel::CreateAssetRow)
  //                 .OnSelectionChanged(this, &CesiumIonPanel::AssetSelected)
  //                 .HeaderRow(
  //                     SNew(SHeaderRow) +
  //                     SHeaderRow::Column(ColumnName_Name)
  //                         .DefaultLabel(FText::FromString(TEXT("Name")))) +
  //                     SHeaderRow::Column(ColumnName_Type)
  //                         .DefaultLabel(FText::FromString(TEXT("Type")))
  //                         .SortMode_Lambda(sortModeLambda(ColumnName_Type))
  //                         .OnSort(FOnSortModeChanged::CreateSP(
  //                             this,
  //                             &CesiumIonPanel::OnSortChange)) +
  //                     SHeaderRow::Column(ColumnName_DateAdded)
  //                         .DefaultLabel(FText::FromString(TEXT("Date
  //                         added"))) .SortMode_Lambda(
  //                             sortModeLambda(ColumnName_DateAdded))
  //                         .OnSort(FOnSortModeChanged::CreateSP(
  //                             this,
  //                             &CesiumIonPanel::OnSortChange))]
  //         .HeaderContent()[SNew(STextBlock).Text(FText::FromString(className))];
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
                          .Text(FText::FromString("glTF Metadata"))]];
}

void CesiumFeaturesMetadataViewer::gatherTilesetStatistics() {
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
            "min",
            FCesiumMetadataValue::fromJsonValue(
                valueType,
                *propertyIt.second.min));
        pProperty->statistics.Emplace(std::move(statistic));
      }

      if (propertyIt.second.max) {
        TSharedRef<StatisticView> statistic = MakeShared<StatisticView>(
            pClassId,
            pPropertyId,
            "max",
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

  //// This assumes that the property tables are the same across all models in
  //// the tileset, and that they all have the same schema.
  // for (const UActorComponent* pComponent : this->_pTileset->GetComponents())
  // {
  //   const auto* pPrimitive = Cast<UPrimitiveComponent>(pComponent);
  //   if (!pPrimitive) {
  //     continue;
  //   }

  //  FCesiumModelMetadata modelMetadata =
  //      UCesiumModelMetadataBlueprintLibrary::GetModelMetadata(pPrimitive);

  //  AutoFillPropertyTableDescriptions(
  //      this->Description.ModelMetadata.PropertyTables,
  //      modelMetadata);
  //  AutoFillPropertyTextureDescriptions(
  //      this->Description.ModelMetadata.PropertyTextures,
  //      modelMetadata);

  //  TArray<USceneComponent*> childComponents;
  //  pGltf->GetChildrenComponents(false, childComponents);

  //  for (const USceneComponent* pChildComponent : childComponents) {
  //    const auto* pCesiumPrimitive = Cast<ICesiumPrimitive>(pChildComponent);
  //    if (!pCesiumPrimitive) {
  //      continue;
  //    }
  //    const CesiumPrimitiveData& primData =
  //        pCesiumPrimitive->getPrimitiveData();
  //    const FCesiumPrimitiveFeatures& primitiveFeatures = primData.Features;
  //    const TArray<FCesiumPropertyTable>& propertyTables =
  //        UCesiumModelMetadataBlueprintLibrary::GetPropertyTables(
  //            modelMetadata);
  //    const FCesiumPrimitiveFeatures* pInstanceFeatures = nullptr;
  //    const auto* pInstancedComponent =
  //        Cast<UCesiumGltfInstancedComponent>(pChildComponent);
  //    if (pInstancedComponent) {
  //      pInstanceFeatures = pInstancedComponent->pInstanceFeatures.Get();
  //    }
  //    AutoFillFeatureIdSetDescriptions(
  //        this->Description.PrimitiveFeatures.FeatureIdSets,
  //        primitiveFeatures,
  //        pInstanceFeatures,
  //        propertyTables);

  //    const FCesiumPrimitiveMetadata& primitiveMetadata = primData.Metadata;
  //    const TArray<FCesiumPropertyTexture>& propertyTextures =
  //        UCesiumModelMetadataBlueprintLibrary::GetPropertyTextures(
  //            modelMetadata);
  //    AutoFillPropertyTextureNames(
  //        this->Description.PrimitiveMetadata.PropertyTextureNames,
  //        primitiveMetadata,
  //        propertyTextures);
  //  }
  // }}
}

void CesiumFeaturesMetadataViewer::registerStatistic(
    TSharedRef<StatisticView> pItem) {
  if (!this->_pTileset.IsValid()) {
    return;
  }

  ACesium3DTileset& tileset = *this->_pTileset;
  UCesiumFeaturesMetadataComponent* pFeaturesMetadata =
      tileset.GetComponentByClass<UCesiumFeaturesMetadataComponent>();

  if (!pFeaturesMetadata) {
    UActorComponent* pActorComponent = tileset.AddComponentByClass(
        UCesiumFeaturesMetadataComponent::StaticClass(),
        false,
        FTransform::Identity,
        false);
    pFeaturesMetadata = Cast<UCesiumFeaturesMetadataComponent>(pActorComponent);
  }

  if (!pFeaturesMetadata) {
    return;
  }

  TMap<FString, FCesiumMetadataClassStatisticsDescription>& statistics =
      pFeaturesMetadata->Description.Statistics;

  FCesiumMetadataClassStatisticsDescription* pClass =
      statistics.Find(*pItem->pClassId);
  if (!pClass) {
    pClass = &statistics.Emplace(*pItem->pClassId);
  }

  FCesiumMetadataPropertyStatisticsDescription* pProperty = nullptr;
  for (FCesiumMetadataPropertyStatisticsDescription& property :
       pClass->Properties) {
    if (property.Id == *pItem->pPropertyId)
      pProperty = &property;
    break;
  }

  if (!pProperty) {
    int32 index = pClass->Properties.Emplace(*pItem->pPropertyId);
    pProperty = &pClass->Properties[index];
  }

  if (pProperty->Values.Find(pItem->id)) {
    pProperty->Values[pItem->id] = pItem->value;
  } else {
    pProperty->Values.Emplace(pItem->id, pItem->value);
  }
}
