// Copyright 2020-2026 CesiumGS, Inc. and Contributors

#include "CesiumVoxelShaderBuilder.h"

#include "Cesium3DTileset.h"
#include "CesiumCommon.h"
#include "CesiumEditor.h"
#include "CesiumMetadataEncodingDetails.h"
#include "CesiumModelMetadata.h"
#include "CesiumPrimitiveFeatures.h"
#include "CesiumPrimitiveMetadata.h"
#include "CesiumRuntimeSettings.h"
#include "CesiumVoxelMetadataComponent.h"
#include "Containers/LazyPrintf.h"
#include "EditorStyleSet.h"
#include "LevelEditor.h"
#include "PropertyCustomizationHelpers.h"
#include "ScopedTransaction.h"
#include "Text/HLSLSyntaxHighlighterMarshaller.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SHeader.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"

THIRD_PARTY_INCLUDES_START
#include <Cesium3DTiles/ExtensionContent3dTilesContentVoxels.h>
#include <Cesium3DTiles/Statistics.h>
#include <Cesium3DTilesSelection/Tileset.h>
#include <Cesium3DTilesSelection/TilesetMetadata.h>
#include <CesiumUtility/Uri.h>
THIRD_PARTY_INCLUDES_END

/*static*/ TSharedPtr<CesiumVoxelShaderBuilder>
    CesiumVoxelShaderBuilder::_pExistingWindow = nullptr;

/*static*/ TMap<FString, TSharedRef<FString>>
    CesiumVoxelShaderBuilder::_stringMap = {};

/*static*/ void
CesiumVoxelShaderBuilder::Open(TWeakObjectPtr<ACesium3DTileset> pTileset) {
  if (_pExistingWindow.IsValid()) {
    _pExistingWindow->_pTileset = pTileset;
  } else {
    // Open a new panel
    TSharedRef<CesiumVoxelShaderBuilder> pWindow =
        SNew(CesiumVoxelShaderBuilder).Tileset(pTileset);

    _pExistingWindow = pWindow;
    _pExistingWindow->GetOnWindowClosedEvent().AddLambda(
        [&pExistingWindow = CesiumVoxelShaderBuilder::_pExistingWindow](
            const TSharedRef<SWindow>& pWindow) { pExistingWindow = nullptr; });

    FSlateApplication::Get().AddWindow(pWindow);
  }

  if (pTileset.IsValid()) {
    _pExistingWindow->_pVoxelMetadataComponent =
        pTileset->GetComponentByClass<UCesiumVoxelMetadataComponent>();
  }

  _pExistingWindow->SyncAndRebuildUI();
  _pExistingWindow->BringToFront();
}

const FString voxelClassTooltip =
    TEXT("The class used by the 3DTILES_content_voxels extension on the "
         "Cesium3DTileset, if present.\n\n"
         "This displays any available properties and statistics, which can "
         "then be added for use in the tileset's custom material.");
const FString customShaderTooltip =
    TEXT("The custom shader for styling the voxel metadata.\n\n"
         "Voxels are rendered by raycasting through visible cells, "
         "accumulating color until the result is opaque. This will be "
         "the code that is executed for each iteration, returning a "
         "color computed based on the metadata at a given cell.");
const FString additionalFunctionsTooltip =
    TEXT("Any additional functions to include with the custom shader to style "
         "the voxel metadata.\n\n"
         "This is best suited for helper functions that are repeatedly used "
         "in the custom shader, such as statistics-based operations or "
         "color ramps.");
const FString shaderPreviewTooltip =
    TEXT("A preview of how the custom shader will appear in the "
         "generated material, based on the fields on the left.\n\n"
         "Use Generate Material to apply your changes to the acutal material.");

void CesiumVoxelShaderBuilder::Construct(const FArguments& InArgs) {
  const TWeakObjectPtr<ACesium3DTileset>& pTileset = InArgs._Tileset;
  FString label =
      pTileset.IsValid() ? pTileset->GetActorLabel() : TEXT("Unknown");
  this->_pTileset = pTileset;

  // Create the syntax highlighter
  FHLSLSyntaxHighlighterMarshaller::FSyntaxTextStyle style(
      FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>(
          "SyntaxHighlight.SourceCode.Normal"),
      FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>(
          "SyntaxHighlight.SourceCode.Operator"),
      FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>(
          "SyntaxHighlight.SourceCode.Keyword"),
      FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>(
          "SyntaxHighlight.SourceCode.String"),
      FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>(
          "SyntaxHighlight.SourceCode.Number"),
      FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>(
          "SyntaxHighlight.SourceCode.Comment"),
      FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>(
          "SyntaxHighlight.SourceCode.PreProcessorKeyword"),
      FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>(
          "SyntaxHighlight.SourceCode.Error"));
  this->_pSyntaxHighlighter = FHLSLSyntaxHighlighterMarshaller::Create(style);

  SAssignNew(this->_pVoxelClassContent, SScrollBox);
  SAssignNew(this->_pCustomShader, SMultiLineEditableTextBox)
      .AutoWrapText(false)
      .Margin(FMargin(5, 5, 5, 5))
      .Text_Lambda([&pComponent = this->_pVoxelMetadataComponent]() {
        if (pComponent.IsValid()) {
          return FText::FromString(pComponent->CustomShader);
        } else {
          return FText();
        }
      })
      .OnTextCommitted(FOnTextCommitted::CreateLambda(
          [](const FText& InText, ETextCommit::Type InType) {
            if (!_pExistingWindow)
              return;
            const TWeakObjectPtr<UCesiumVoxelMetadataComponent>& pComponent =
                _pExistingWindow->_pVoxelMetadataComponent;
            if (pComponent.IsValid()) {
              pComponent->CustomShader = InText.ToString();
              _pExistingWindow->_customShaderPreview =
                  pComponent->getCustomShaderPreview();
            }
          }))
      .Marshaller(this->_pSyntaxHighlighter);

  SAssignNew(this->_pAdditionalFunctions, SMultiLineEditableTextBox)
      .AutoWrapText(false)
      .Margin(FMargin(5, 5, 5, 5))
      .Text_Lambda([&pComponent = this->_pVoxelMetadataComponent]() {
        if (pComponent.IsValid()) {
          return FText::FromString(pComponent->AdditionalFunctions);
        } else {
          return FText();
        }
      })
      .OnTextCommitted(FOnTextCommitted::CreateLambda(
          [](const FText& InText, ETextCommit::Type InType) {
            if (!_pExistingWindow)
              return;
            const TWeakObjectPtr<UCesiumVoxelMetadataComponent>& pComponent =
                _pExistingWindow->_pVoxelMetadataComponent;
            if (pComponent.IsValid()) {
              pComponent->AdditionalFunctions = InText.ToString();
              _pExistingWindow->_customShaderPreview =
                  pComponent->getCustomShaderPreview();
            }
          }))
      .Marshaller(this->_pSyntaxHighlighter);

  TSharedRef<SScrollBox> pLeft = SNew(SScrollBox);
  pLeft->AddSlot()
      [SNew(SHeader)
           .Content()[SNew(STextBlock)
                          .TextStyle(FCesiumEditorModule::GetStyle(), "Heading")
                          .Text(FText::FromString(TEXT("Voxel Class")))
                          .ToolTipText(FText::FromString(voxelClassTooltip))
                          .Margin(FMargin(0.f, 10.f))]];
  pLeft->AddSlot()[this->_pVoxelClassContent.ToSharedRef()];
  pLeft->AddSlot()
      [SNew(SHeader)
           .Content()[SNew(STextBlock)
                          .TextStyle(FCesiumEditorModule::GetStyle(), "Heading")
                          .Text(FText::FromString(TEXT("Custom Shader")))
                          .ToolTipText(FText::FromString(customShaderTooltip))
                          .Margin(FMargin(0.f, 10.f))]];
  pLeft->AddSlot()[this->_pCustomShader.ToSharedRef()];
  pLeft->AddSlot()
      [SNew(SHeader).Content()
           [SNew(STextBlock)
                .TextStyle(FCesiumEditorModule::GetStyle(), "Heading")
                .Text(FText::FromString(TEXT("Additional Functions")))
                .ToolTipText(FText::FromString(additionalFunctionsTooltip))
                .Margin(FMargin(0.f, 10.f))]];
  pLeft->AddSlot()[this->_pAdditionalFunctions.ToSharedRef()];
  pLeft->AddSlot()
      .Padding(0.0f, 10.0f)
      .VAlign(VAlign_Bottom)
      .HAlign(HAlign_Center)
          [SNew(SButton)
               .ButtonStyle(FCesiumEditorModule::GetStyle(), "CesiumButton")
               .TextStyle(FCesiumEditorModule::GetStyle(), "CesiumButtonText")
               .ContentPadding(FMargin(1.0, 1.0))
               .HAlign(EHorizontalAlignment::HAlign_Center)
               .Text(FText::FromString(TEXT("Refresh")))
               .ToolTipText(FText::FromString(TEXT(
                   "Refreshes for the voxel class on the ACesium3DTileset, if present.")))
               .OnClicked_Lambda([this]() {
                 this->SyncAndRebuildUI();
                 return FReply::Handled();
               })];

  SAssignNew(this->_pShaderPreview, SMultiLineEditableTextBox)
      .AutoWrapText(true)
      .IsReadOnly(true)
      .Margin(FMargin(5, 5, 5, 5))
      .Text_Lambda([]() {
        if (_pExistingWindow) {
          return FText::FromString(_pExistingWindow->_customShaderPreview);
        } else {
          return FText();
        }
      })
      .Marshaller(this->_pSyntaxHighlighter);

  TSharedRef<SScrollBox> pRight = SNew(SScrollBox);
  pRight->AddSlot()
      [SNew(SHeader)
           .Content()[SNew(STextBlock)
                          .TextStyle(FCesiumEditorModule::GetStyle(), "Heading")
                          .Text(FText::FromString(TEXT("Shader Preview")))
                          .Margin(FMargin(0.f, 10.f))]];
  pRight->AddSlot()[this->_pShaderPreview.ToSharedRef()];

  TSharedRef<SSplitter> content =
      SNew(SSplitter)
          .Orientation(EOrientation::Orient_Horizontal)
          .PhysicalSplitterHandleSize(2.0f);
  content->AddSlot().Value(0.6f)[pLeft];
  content->AddSlot().Value(0.4f)[pRight];

  SWindow::Construct(
      SWindow::FArguments()
          .Title(FText::FromString(
              FString::Format(TEXT("{0}: Voxel Metadata Shader"), {label})))
          .AutoCenter(EAutoCenter::PreferredWorkArea)
          .SizingRule(ESizingRule::UserSized)
          .ClientSize(FVector2D(
              1200,
              800))[SNew(SBorder)
                        .Visibility(EVisibility::Visible)
                        .BorderImage(FAppStyle::GetBrush("Menu.Background"))
                        .Padding(FMargin(10.0f))[content]]);
}

void CesiumVoxelShaderBuilder::SyncAndRebuildUI() {
  this->_pVoxelClass.Reset();
  this->_stringMap.Empty();

  TSharedRef<SScrollBox> pContent = this->_pVoxelClassContent.ToSharedRef();
  pContent->ClearChildren();

  this->gatherVoxelPropertiesAndStatistics();
  this->createVoxelClassDropdown();

  this->_customShaderPreview =
      this->_pVoxelMetadataComponent.IsValid()
          ? this->_pVoxelMetadataComponent->getCustomShaderPreview()
          : FString();
}

void CesiumVoxelShaderBuilder::gatherVoxelPropertiesAndStatistics() {
  if (!this->_pTileset.IsValid())
    return;

  ACesium3DTileset& tileset = *this->_pTileset;
  const Cesium3DTilesSelection::Tileset* pTileset = tileset.GetTileset();
  if (!pTileset || !pTileset->getRootTile())
    return;

  const Cesium3DTilesSelection::TileExternalContent* pExternalContent =
      pTileset->getRootTile()->getContent().getExternalContent();
  if (!pExternalContent)
    return;

  const auto* pVoxelExtension =
      pExternalContent
          ->getExtension<Cesium3DTiles::ExtensionContent3dTilesContentVoxels>();
  if (!pVoxelExtension) {
    UE_LOG(
        LogCesiumEditor,
        Warning,
        TEXT(
            "Tileset %s does not contain voxel content, so CesiumVoxelMetadataComponent will have no effect."),
        *this->_pTileset->GetName());
    return;
  }

  const Cesium3DTilesSelection::TilesetMetadata* pMetadata =
      pTileset->getMetadata();
  if (!pMetadata || !pMetadata->schema) {
    UE_LOG(
        LogCesiumEditor,
        Error,
        TEXT(
            "Tileset {} does not contain a metadata schema, which is required for voxel content."),
        *tileset.GetActorLabel());
    return;
  }

  const std::string& voxelClassId = pVoxelExtension->classProperty;
  if (pMetadata->schema->classes.find(voxelClassId) ==
      pMetadata->schema->classes.end()) {
    UE_LOG(
        LogCesiumEditor,
        Error,
        TEXT(
            "Tileset {} does not the voxel class required by 3DTILES_content_voxel."),
        *tileset.GetActorLabel());
    return;
  }

  const Cesium3DTiles::Class& voxelClass =
      pMetadata->schema->classes.at(voxelClassId);

  std::optional<Cesium3DTiles::ClassStatistics> statistics = std::nullopt;
  if (pMetadata->statistics &&
      pMetadata->statistics->classes.contains(voxelClassId)) {
    statistics = pMetadata->statistics->classes.at(voxelClassId);
  }

  TSharedRef<FString> pClassId = getSharedRef(FString(voxelClassId.c_str()));
  TArray<TSharedRef<VoxelProperty>> properties;

  for (const auto& propertyIt : voxelClass.properties) {
    FCesiumMetadataValueType valueType =
        FCesiumMetadataValueType::fromClassProperty(propertyIt.second);

    FString propertyId = propertyIt.first.c_str();

    VoxelProperty property{.pId = getSharedRef(propertyId)};

    FCesiumMetadataPropertyDetails& propertyDetails = property.propertyDetails;
    propertyDetails.SetValueType(valueType);
    propertyDetails.ArraySize = propertyIt.second.count.value_or(0);
    propertyDetails.bIsNormalized = propertyIt.second.normalized;

    // These values are not actually validated until the material is generated.
    propertyDetails.bHasOffset = propertyIt.second.offset.has_value();
    propertyDetails.bHasScale = propertyIt.second.scale.has_value();
    propertyDetails.bHasNoDataValue = propertyIt.second.noData.has_value();
    propertyDetails.bHasDefaultValue =
        propertyIt.second.defaultProperty.has_value();

    FCesiumMetadataEncodingDetails bestFitEncodingDetails =
        FCesiumMetadataEncodingDetails::GetBestFitForProperty(propertyDetails);

    // Do some silly TSharedRef lookup since it's required by SComboBox.
    TArray<TSharedRef<ECesiumEncodedMetadataConversion>> supportedConversions =
        ConversionEnum.getSharedRefs(
            FCesiumMetadataEncodingDetails::GetSupportedConversionsForProperty(
                property.propertyDetails));
    property.encodingDetails.conversionMethods =
        std::move(supportedConversions);

    if (statistics && statistics->properties.contains(propertyIt.first)) {
      const Cesium3DTiles::PropertyStatistics& propertyStatistics =
          statistics->properties.at(propertyIt.first);
      TArray<TSharedRef<StatisticView>> statisticsView;

      auto syncStatistic =
          [&statisticsView, &pClassId, &pPropertyId = property.pId, &valueType](
              ECesiumMetadataStatisticSemantic semantic,
              const std::optional<CesiumUtility::JsonValue>& maybeValue) {
            if (maybeValue) {
              StatisticView statistic{
                  pClassId,
                  pPropertyId,
                  semantic,
                  FCesiumMetadataValue(*maybeValue, valueType)};
              statisticsView.Emplace(
                  MakeShared<StatisticView>(std::move(statistic)));
            }
          };

      syncStatistic(
          ECesiumMetadataStatisticSemantic::Min,
          propertyStatistics.min);
      syncStatistic(
          ECesiumMetadataStatisticSemantic::Max,
          propertyStatistics.max);
      syncStatistic(
          ECesiumMetadataStatisticSemantic::Mean,
          propertyStatistics.mean);
      syncStatistic(
          ECesiumMetadataStatisticSemantic::Median,
          propertyStatistics.median);
      syncStatistic(
          ECesiumMetadataStatisticSemantic::StandardDeviation,
          propertyStatistics.standardDeviation);
      syncStatistic(
          ECesiumMetadataStatisticSemantic::Variance,
          propertyStatistics.variance);
      syncStatistic(
          ECesiumMetadataStatisticSemantic::Sum,
          propertyStatistics.sum);
      property.statistics = std::move(statisticsView);

      properties.Add(MakeShared<VoxelProperty>(std::move(property)));
    }
  }

  VoxelClass classView{.pId = pClassId, .properties = std::move(properties)};

  this->_pVoxelClass = MakeUnique<VoxelClass>(std::move(classView));
}

void CesiumVoxelShaderBuilder::syncPropertyEncodingDetails() {
  if (!this->_pVoxelMetadataComponent.IsValid() ||
      !this->_pVoxelClass.IsValid())
    return;

  const FCesiumVoxelClassDescription& description =
      this->_pVoxelMetadataComponent->Description;
  if (description.ClassId != *this->_pVoxelClass->pId)
    return;

  for (const FCesiumPropertyAttributePropertyDescription& property :
       description.Properties) {
    const TSharedRef<VoxelProperty>* ppPropertyView =
        this->_pVoxelClass->properties.FindByPredicate(
            [&name = property.Name](
                const TSharedRef<VoxelProperty>& pPropertyView) {
              return *pPropertyView->pId == name;
            });
    if (!ppPropertyView) {
      continue;
    }

    VoxelProperty& propertyView = **ppPropertyView;
    auto& conversionMethods = propertyView.encodingDetails.conversionMethods;
    int32 conversionIndex = INDEX_NONE;
    for (int32 i = 0; i < conversionMethods.Num(); i++) {
      if (*conversionMethods[i] == property.EncodingDetails.Conversion) {
        conversionIndex = i;
        break;
      }
    }

    if (conversionIndex == INDEX_NONE) {
      // This conversion method isn't supported for the property;
      continue;
    }

    int64 typeIndex = int64(property.EncodingDetails.Type);
    int64 componentTypeIndex = int64(property.EncodingDetails.ComponentType);

    // Here the combo boxes will still be nullptr because
    // OnGenerateRow is not executed right away, so save the selected
    // options.
    propertyView.encodingDetails.pConversionSelection =
        conversionMethods[conversionIndex].ToSharedPtr();
    propertyView.encodingDetails.pEncodedTypeSelection =
        EncodedTypeEnum.options[typeIndex].ToSharedPtr();
    propertyView.encodingDetails.pEncodedComponentTypeSelection =
        EncodedComponentTypeEnum.options[componentTypeIndex].ToSharedPtr();
  }
}

TSharedRef<ITableRow> CesiumVoxelShaderBuilder::createStatisticRow(
    TSharedRef<StatisticView> pItem,
    const TSharedRef<STableViewBase>& list) {

  TSharedRef<SWidget> pAddButton = PropertyCustomizationHelpers::MakeAddButton(
      FSimpleDelegate::CreateLambda(
          [this, pItem]() { this->registerStatistic(pItem); }),
      FText::FromString(TEXT(
          "Add this property statistic to the tileset's CesiumVoxelMetadataComponent.")),
      TAttribute<bool>::Create([this, pItem]() {
        return this->findOnComponent(pItem) == ComponentSearchResult::NoMatch;
      }));

  TSharedRef<SWidget> pRemoveButton = PropertyCustomizationHelpers::MakeRemoveButton(
      FSimpleDelegate::CreateLambda(
          [this, pItem]() { this->removeStatistic(pItem); }),
      FText::FromString(TEXT(
          "Remove this property statistic from the tileset's CesiumVoxelMetadataComponent")),
      TAttribute<bool>::Create([this, pItem]() {
        return this->findOnComponent(pItem) != ComponentSearchResult::NoMatch;
      }));

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
                                 MetadataEnumUtility<
                                     ECesiumMetadataStatisticSemantic>::
                                     enumToNameString(pItem->semantic)))] +
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
                        .VAlign(EVerticalAlignment::VAlign_Center)[pAddButton] +
                    SHorizontalBox::Slot()
                        .AutoWidth()
                        .HAlign(EHorizontalAlignment::HAlign_Right)
                        .VAlign(
                            EVerticalAlignment::VAlign_Center)[pRemoveButton]]];
}

TSharedRef<SListView<TSharedRef<StatisticView>>>
CesiumVoxelShaderBuilder::createPropertyStatisticsList(
    TSharedRef<VoxelProperty> pItem) {
  return SNew(SListView<TSharedRef<StatisticView>>)
      .ListItemsSource(&pItem->statistics)
      .SelectionMode(ESelectionMode::None)
      .OnGenerateRow(this, &CesiumVoxelShaderBuilder::createStatisticRow);
}

template <typename TEnum>
TSharedRef<SWidget>
CesiumVoxelShaderBuilder::createEnumDropdownOption(TSharedRef<TEnum> pOption) {
  return SNew(STextBlock)
      .Text(MetadataEnumUtility<TEnum>::getEnumDisplayNameText(*pOption));
}

template <typename TEnum>
void CesiumVoxelShaderBuilder::createEnumComboBox(
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
          &CesiumVoxelShaderBuilder::createEnumDropdownOption<TEnum>)
      .Content()[SNew(STextBlock)
                     .MinDesiredWidth(50.0f)
                     .Text_Lambda([&pComboBox]() {
                       return pComboBox->GetSelectedItem().IsValid()
                                  ? MetadataEnumUtility<TEnum>::
                                        getEnumDisplayNameText(
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

TSharedRef<SBox>
CesiumVoxelShaderBuilder::createPropertyRow(TSharedRef<VoxelProperty> pItem) {
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
                   "The type of the property as defined in the metadata schema.")))] +
      SHorizontalBox::Slot()
          .AutoWidth()
          .MaxWidth(200.0f)
          .Padding(5.0f)
          .HAlign(EHorizontalAlignment::HAlign_Left)
          .VAlign(EVerticalAlignment::VAlign_Center)
              [SNew(STextBlock)
                   .AutoWrapText(true)
                   .Text(FText::FromString(qualifierString))
                   .ToolTipText(FText::FromString(
                       "Notable qualities of the property that require additional nodes to be generated for the material."))];

  FCesiumMetadataEncodingDetails bestFitEncodingDetails =
      FCesiumMetadataEncodingDetails::GetBestFitForProperty(
          pItem->propertyDetails);

  this->createEnumComboBox(
      pItem->encodingDetails.pConversionCombo,
      ConversionEnum.options,
      pItem->encodingDetails.pConversionSelection
          ? *pItem->encodingDetails.pConversionSelection
          : bestFitEncodingDetails.Conversion,
      FString());

  this->createEnumComboBox(
      pItem->encodingDetails.pEncodedTypeCombo,
      EncodedTypeEnum.options,
      pItem->encodingDetails.pEncodedTypeSelection
          ? *pItem->encodingDetails.pEncodedTypeSelection
          : bestFitEncodingDetails.Type,
      TEXT(
          "The type to which to coerce the property's data. Affects the texture format that is used to encode the data."));

  this->createEnumComboBox<ECesiumEncodedMetadataComponentType>(
      pItem->encodingDetails.pEncodedComponentTypeCombo,
      EncodedComponentTypeEnum.options,
      pItem->encodingDetails.pEncodedComponentTypeSelection
          ? *pItem->encodingDetails.pEncodedComponentTypeSelection
          : bestFitEncodingDetails.ComponentType,
      TEXT(
          "The component type to which to coerce the property's data. Affects the texture format that is used to encode the data."));

  if (pItem->encodingDetails.pConversionCombo.IsValid()) {
    content->AddSlot().FillWidth(0.6).Padding(5.0f).VAlign(
        EVerticalAlignment::VAlign_Center)[pItem->encodingDetails
                                               .pConversionCombo->AsShared()];
  }

  auto visibilityLambda = TAttribute<EVisibility>::Create([pItem]() {
    bool show = false;
    if (pItem->encodingDetails.pConversionCombo.IsValid()) {
      show =
          pItem->encodingDetails.pConversionCombo->GetSelectedItem().IsValid();
    }
    return show ? EVisibility::Visible : EVisibility::Hidden;
  });

  if (pItem->encodingDetails.pEncodedTypeCombo.IsValid()) {
    pItem->encodingDetails.pEncodedTypeCombo->SetVisibility(visibilityLambda);
    content->AddSlot().AutoWidth().Padding(5.0f).VAlign(
        EVerticalAlignment::VAlign_Center)[pItem->encodingDetails
                                               .pEncodedTypeCombo->AsShared()];
  }

  if (pItem->encodingDetails.pEncodedComponentTypeCombo.IsValid()) {
    pItem->encodingDetails.pEncodedComponentTypeCombo->SetVisibility(
        visibilityLambda);
    content->AddSlot().AutoWidth().Padding(5.0f).VAlign(
        EVerticalAlignment::VAlign_Center)
        [pItem->encodingDetails.pEncodedComponentTypeCombo->AsShared()];
  }

  TSharedRef<SWidget> pAddButton = PropertyCustomizationHelpers::MakeAddButton(
      FSimpleDelegate::CreateLambda(
          [this, pItem]() { this->registerProperty(pItem); }),
      FText::FromString(TEXT(
          "Add this property to the tileset's CesiumVoxelMetadataComponent.")),
      TAttribute<bool>::Create([this, pItem]() {
        FCesiumMetadataEncodingDetails selectedEncodingDetails =
            getSelectedEncodingDetails(
                pItem->encodingDetails.pConversionCombo,
                pItem->encodingDetails.pEncodedTypeCombo,
                pItem->encodingDetails.pEncodedComponentTypeCombo);
        return this->findOnComponent(pItem, false) ==
                   ComponentSearchResult::NoMatch &&
               validateEncodingDetails(selectedEncodingDetails);
      }));
  pAddButton->SetVisibility(TAttribute<EVisibility>::Create([this, pItem]() {
    return this->findOnComponent(pItem, false) == ComponentSearchResult::NoMatch
               ? EVisibility::Visible
               : EVisibility::Collapsed;
  }));

  TSharedRef<SWidget> pOverwriteButton =
      PropertyCustomizationHelpers::MakeEditButton(
          FSimpleDelegate::CreateLambda(
              [this, pItem]() { this->registerProperty(pItem); }),
          FText::FromString(TEXT(
              "Overwrites the existing property on the tileset's CesiumVoxelMetadataComponent "
              "with the same name.")),
          TAttribute<bool>::Create([this, pItem]() {
            FCesiumMetadataEncodingDetails selectedEncodingDetails =
                getSelectedEncodingDetails(
                    pItem->encodingDetails.pConversionCombo,
                    pItem->encodingDetails.pEncodedTypeCombo,
                    pItem->encodingDetails.pEncodedComponentTypeCombo);
            return this->findOnComponent(pItem, true) ==
                       ComponentSearchResult::PartialMatch &&
                   validateEncodingDetails(selectedEncodingDetails);
          }));
  pOverwriteButton->SetVisibility(TAttribute<EVisibility>::Create([this,
                                                                   pItem]() {
    return this->findOnComponent(pItem, true) != ComponentSearchResult::NoMatch
               ? EVisibility::Visible
               : EVisibility::Collapsed;
  }));

  TSharedRef<SWidget> pRemoveButton = PropertyCustomizationHelpers::MakeRemoveButton(
      FSimpleDelegate::CreateLambda(
          [this, pItem]() { this->removeProperty(pItem); }),
      FText::FromString(TEXT(
          "Remove this property from the tileset's CesiumVoxelMetadataComponent.")),
      TAttribute<bool>::Create([this, pItem]() {
        return this->findOnComponent(pItem, false) ==
               ComponentSearchResult::ExactMatch;
      }));

  content->AddSlot()
      .AutoWidth()
      .HAlign(EHorizontalAlignment::HAlign_Right)
      .VAlign(EVerticalAlignment::VAlign_Center)[pAddButton];
  content->AddSlot()
      .AutoWidth()
      .HAlign(EHorizontalAlignment::HAlign_Right)
      .VAlign(EVerticalAlignment::VAlign_Center)[pOverwriteButton];
  content->AddSlot()
      .AutoWidth()
      .HAlign(EHorizontalAlignment::HAlign_Right)
      .VAlign(EVerticalAlignment::VAlign_Center)[pRemoveButton];

  return SNew(SBox)
      .HAlign(EHorizontalAlignment::HAlign_Fill)
      .Content()[content];
}

TSharedRef<ITableRow> CesiumVoxelShaderBuilder::createVoxelPropertyDropdown(
    TSharedRef<VoxelProperty> pItem,
    const TSharedRef<STableViewBase>& list) {
  TSharedRef<SVerticalBox> pPropertyRow = SNew(SVerticalBox);
  pPropertyRow->AddSlot()[createPropertyRow(pItem)];
  pPropertyRow->AddSlot()[createPropertyStatisticsList(pItem)].AutoHeight();

  return SNew(STableRow<TSharedRef<VoxelProperty>>, list)
      .Content()[SNew(SExpandableArea)
                     .InitiallyCollapsed(true)
                     .HeaderContent()[SNew(STextBlock)
                                          .Text(FText::FromString(*pItem->pId))]
                     .BodyContent()[pPropertyRow]];
}

void CesiumVoxelShaderBuilder::createVoxelClassDropdown() {
  if (!this->_pVoxelClass.IsValid()) {
    this->_pVoxelClassContent->AddSlot()
        [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(0.05f) +
         SHorizontalBox::Slot()
             [SNew(STextBlock)
                  .AutoWrapText(true)
                  .Text(FText::FromString(TEXT(
                      "This tileset does not contain a valid voxel class.")))]];
    return;
  }

  this->_pVoxelClassContent->AddSlot()
      [SNew(SExpandableArea)
           .InitiallyCollapsed(false)
           .HeaderContent()[SNew(STextBlock)
                                .Text(FText::FromString(
                                    *this->_pVoxelClass->pId))]
           .BodyContent()
               [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(0.05f) +
                SHorizontalBox::Slot()
                    [SNew(SListView<TSharedRef<VoxelProperty>>)
                         .ListItemsSource(&this->_pVoxelClass->properties)
                         .SelectionMode(ESelectionMode::None)
                         .OnGenerateRow(
                             this,
                             &CesiumVoxelShaderBuilder::
                                 createVoxelPropertyDropdown)]]];
}

namespace {
FCesiumMetadataPropertyStatisticsDescription* findPropertyStatistic(
    TArray<FCesiumMetadataPropertyStatisticsDescription>& statistics,
    const FString& propertyId,
    bool createIfMissing) {
  FCesiumMetadataPropertyStatisticsDescription* pProperty =
      statistics.FindByPredicate(
          [&propertyId](const FCesiumMetadataPropertyStatisticsDescription&
                            existingProperty) {
            return existingProperty.Id == propertyId;
          });

  if (!pProperty && createIfMissing) {
    int32 index = statistics.Emplace(
        FCesiumMetadataPropertyStatisticsDescription{propertyId});
    pProperty = &statistics[index];
  }

  return pProperty;
}

FCesiumPropertyAttributePropertyDescription* findProperty(
    TArray<FCesiumPropertyAttributePropertyDescription>& properties,
    const FString& propertyName,
    bool createIfMissing) {
  FCesiumPropertyAttributePropertyDescription* pProperty =
      properties.FindByPredicate(
          [&propertyName](const FCesiumPropertyAttributePropertyDescription&
                              existingProperty) {
            return propertyName == existingProperty.Name;
          });

  if (!pProperty && createIfMissing) {
    int32 index = properties.Emplace();
    pProperty = &properties[index];
    pProperty->Name = propertyName;
  }

  return pProperty;
}

} // namespace

ComponentSearchResult CesiumVoxelShaderBuilder::findOnComponent(
    TSharedRef<StatisticView> pItem) const {
  if (!this->_pVoxelMetadataComponent.IsValid()) {
    return ComponentSearchResult::NoMatch;
  }

  UCesiumVoxelMetadataComponent& voxelMetadata =
      *this->_pVoxelMetadataComponent;

  TArray<FCesiumMetadataPropertyStatisticsDescription>& statistics =
      voxelMetadata.Description.Statistics;

  FCesiumMetadataPropertyStatisticsDescription* pProperty =
      findPropertyStatistic(statistics, *pItem->pPropertyId, false);
  FCesiumMetadataPropertyStatisticValue* pValue = nullptr;

  if (pProperty) {
    pValue = pProperty->Values.FindByPredicate(
        [semantic = pItem->semantic](
            const FCesiumMetadataPropertyStatisticValue& existingValue) {
          return existingValue.Semantic == semantic;
        });
  }

  return pValue ? ComponentSearchResult::ExactMatch
                : ComponentSearchResult::NoMatch;
}

ComponentSearchResult CesiumVoxelShaderBuilder::findOnComponent(
    TSharedRef<VoxelProperty> pItem,
    bool compareEncodingDetails) const {
  if (!this->_pVoxelMetadataComponent.IsValid()) {
    return ComponentSearchResult::NoMatch;
  }

  UCesiumVoxelMetadataComponent& voxelMetadata =
      *this->_pVoxelMetadataComponent;

  // Check whether the property already exists.
  const FCesiumPropertyAttributePropertyDescription* pProperty =
      findProperty(voxelMetadata.Description.Properties, *pItem->pId, false);

  if (!pProperty) {
    return ComponentSearchResult::NoMatch;
  }

  if (pProperty->PropertyDetails != pItem->propertyDetails) {
    return ComponentSearchResult::PartialMatch;
  }

  if (compareEncodingDetails) {
    FCesiumMetadataEncodingDetails selectedEncodingDetails =
        getSelectedEncodingDetails(
            pItem->encodingDetails.pConversionCombo,
            pItem->encodingDetails.pEncodedTypeCombo,
            pItem->encodingDetails.pEncodedComponentTypeCombo);

    return pProperty->EncodingDetails == selectedEncodingDetails
               ? ComponentSearchResult::ExactMatch
               : ComponentSearchResult::PartialMatch;
  } else {
    return ComponentSearchResult::ExactMatch;
  }
}

void CesiumVoxelShaderBuilder::registerStatistic(
    TSharedRef<StatisticView> pItem) {
  if (!this->_pVoxelMetadataComponent.IsValid()) {
    UE_LOG(
        LogCesiumEditor,
        Error,
        TEXT(
            "This window was opened for a now invalid CesiumVoxelMetadataComponent."))
    return;
  }

  FCesiumVoxelClassDescription& description =
      this->_pVoxelMetadataComponent->Description;
  TArray<FCesiumMetadataPropertyStatisticsDescription>& statistics =
      description.Statistics;

  FCesiumMetadataPropertyStatisticsDescription* pProperty =
      findPropertyStatistic(statistics, *pItem->pPropertyId, true);
  CESIUM_ASSERT(pProperty != nullptr);

  UKismetSystemLibrary::BeginTransaction(
      TEXT("Cesium Voxel Shader Builder"),
      FText::FromString(FString("Add voxel statistic to ACesium3DTileset")),
      this->_pVoxelMetadataComponent.Get());
  this->_pVoxelMetadataComponent->PreEditChange(NULL);

  if (FCesiumMetadataPropertyStatisticValue* pValue =
          pProperty->Values.FindByPredicate(
              [semantic = pItem->semantic](
                  const FCesiumMetadataPropertyStatisticValue& existingValue) {
                return existingValue.Semantic == semantic;
              });
      pValue != nullptr) {
    pValue->Value = pItem->value;
  } else {
    pProperty->Values.Emplace(
        FCesiumMetadataPropertyStatisticValue{pItem->semantic, pItem->value});
  }

  description.ClassId = *this->_pVoxelClass->pId;

  this->_pVoxelMetadataComponent->PostEditChange();
  UKismetSystemLibrary::EndTransaction();

  // Make sure to update the shader preview for the new statistic.
  _pExistingWindow->_customShaderPreview =
      this->_pVoxelMetadataComponent->getCustomShaderPreview();
}

void CesiumVoxelShaderBuilder::registerProperty(
    TSharedRef<VoxelProperty> pItem) {
  if (!this->_pVoxelMetadataComponent.IsValid()) {
    UE_LOG(
        LogCesiumEditor,
        Error,
        TEXT(
            "This window was opened for a now invalid CesiumVoxelMetadataComponent."))
    return;
  }

  UKismetSystemLibrary::BeginTransaction(
      TEXT("Cesium Voxel Shader Builder"),
      FText::FromString(
          FString("Register voxel property with ACesium3DTileset")),
      this->_pVoxelMetadataComponent.Get());
  this->_pVoxelMetadataComponent->PreEditChange(NULL);

  FCesiumVoxelClassDescription& description =
      this->_pVoxelMetadataComponent->Description;
  CESIUM_ASSERT(
      pItem->encodingDetails->pConversionCombo &&
      pItem->encodingDetails->pEncodedTypeCombo &&
      pItem->encodingDetails->pEncodedComponentTypeCombo);

  FCesiumPropertyAttributePropertyDescription* pProperty =
      findProperty(description.Properties, *pItem->pId, true);

  CESIUM_ASSERT(pProperty != nullptr);

  FCesiumPropertyAttributePropertyDescription& property = *pProperty;
  property.PropertyDetails = pItem->propertyDetails;
  property.EncodingDetails = getSelectedEncodingDetails(
      pItem->encodingDetails.pConversionCombo,
      pItem->encodingDetails.pEncodedTypeCombo,
      pItem->encodingDetails.pEncodedComponentTypeCombo);

  description.ClassId = *this->_pVoxelClass->pId;

  this->_pVoxelMetadataComponent->PostEditChange();
  UKismetSystemLibrary::EndTransaction();

  // Make sure to update the shader preview for the new property.
  _pExistingWindow->_customShaderPreview =
      this->_pVoxelMetadataComponent->getCustomShaderPreview();
}

void CesiumVoxelShaderBuilder::removeStatistic(
    TSharedRef<StatisticView> pItem) {
  if (!this->_pVoxelMetadataComponent.IsValid()) {
    UE_LOG(
        LogCesiumEditor,
        Error,
        TEXT(
            "This window was opened for a now invalid CesiumVoxelMetadataComponent."))
    return;
  }

  TArray<FCesiumMetadataPropertyStatisticsDescription>& statistics =
      this->_pVoxelMetadataComponent->Description.Statistics;

  int32 propertyIndex = INDEX_NONE;
  for (int32 i = 0; i < statistics.Num(); i++) {
    if (statistics[i].Id == *pItem->pPropertyId) {
      propertyIndex = i;
      break;
    }
  }
  if (propertyIndex == INDEX_NONE) {
    return;
  }
  FCesiumMetadataPropertyStatisticsDescription& propertyStatistics =
      statistics[propertyIndex];

  int32 valueIndex = INDEX_NONE;
  for (int32 i = 0; i < propertyStatistics.Values.Num(); i++) {
    if (propertyStatistics.Values[i].Semantic == pItem->semantic) {
      valueIndex = i;
      break;
    }
  }

  if (valueIndex != INDEX_NONE) {
    UKismetSystemLibrary::BeginTransaction(
        TEXT("Cesium Voxel Shader Builder"),
        FText::FromString(FString("Remove statistic from ACesium3DTileset")),
        this->_pVoxelMetadataComponent.Get());
    this->_pVoxelMetadataComponent->PreEditChange(NULL);

    propertyStatistics.Values.RemoveAt(valueIndex);

    if (propertyStatistics.Values.IsEmpty()) {
      statistics.RemoveAt(propertyIndex);
    }

    this->_pVoxelMetadataComponent->PostEditChange();
    UKismetSystemLibrary::EndTransaction();
  }

  // Make sure to update the shader preview for the removed statistic.
  _pExistingWindow->_customShaderPreview =
      this->_pVoxelMetadataComponent->getCustomShaderPreview();
}

void CesiumVoxelShaderBuilder::removeProperty(TSharedRef<VoxelProperty> pItem) {
  if (!this->_pVoxelMetadataComponent.IsValid()) {
    UE_LOG(
        LogCesiumEditor,
        Error,
        TEXT(
            "This window was opened for a now invalid CesiumVoxelMetadataComponent."))
    return;
  }

  FCesiumVoxelClassDescription& description =
      this->_pVoxelMetadataComponent->Description;

  int32 propertyIndex = INDEX_NONE;
  for (int32 i = 0; i < description.Properties.Num(); i++) {
    if (description.Properties[i].Name == *pItem->pId) {
      propertyIndex = i;
      break;
    }
  }

  if (propertyIndex != INDEX_NONE) {
    UKismetSystemLibrary::BeginTransaction(
        TEXT("Cesium Voxel Shader Builder"),
        FText::FromString(
            FString("Remove voxel property instance from ACesium3DTileset")),
        this->_pVoxelMetadataComponent.Get());
    this->_pVoxelMetadataComponent->PreEditChange(NULL);

    description.Properties.RemoveAt(propertyIndex);

    this->_pVoxelMetadataComponent->PostEditChange();
    UKismetSystemLibrary::EndTransaction();
  }

  // Make sure to update the shader preview for the removed property.
  _pExistingWindow->_customShaderPreview =
      this->_pVoxelMetadataComponent->getCustomShaderPreview();
}

TSharedRef<FString>
CesiumVoxelShaderBuilder::getSharedRef(const FString& string) {
  return _stringMap.Contains(string)
             ? _stringMap[string]
             : _stringMap.Emplace(string, MakeShared<FString>(string));
}
