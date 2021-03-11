// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "IonQuickAddPanel.h"
#include "ACesium3DTileset.h"
#include "CesiumEditor.h"
#include "CesiumIonClient/Connection.h"
#include "CesiumIonRasterOverlay.h"
#include "Editor.h"
#include "PropertyCustomizationHelpers.h"
#include "Styling/SlateStyle.h"
#include "UnrealConversions.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SHeader.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"

using namespace CesiumIonClient;

void IonQuickAddPanel::Construct(const FArguments& InArgs) {
  ChildSlot
      [SNew(SVerticalBox) +
       SVerticalBox::Slot()
           .VAlign(VAlign_Top)
           .HAlign(HAlign_Fill)
           .AutoHeight()
           .Padding(FMargin(5.0f, 20.0f, 5.0f, 10.0f))
               [SNew(SHeader).Content()
                    [SNew(STextBlock)
                         .TextStyle(FCesiumEditorModule::GetStyle(), "Heading")
                         .Text(FText::FromString(TEXT("Quick Add")))]] +
       SVerticalBox::Slot()
           .VAlign(VAlign_Top)
           .HAlign(HAlign_Fill)
           .Padding(FMargin(5.0f, 0.0f, 5.0f, 20.0f))[this->QuickAddList()]];
}

TSharedRef<SWidget> IonQuickAddPanel::QuickAddList() {
  static const TArray<TSharedRef<QuickAddItem>> quickAddItems = {
      MakeShared<QuickAddItem>(QuickAddItem{
          "Cesium World Terrain + Bing Maps Aerial imagery",
          "Cesium World Terrain",
          1,
          "Bing Maps Aerial",
          2}),
      MakeShared<QuickAddItem>(QuickAddItem{
          "Cesium World Terrain + Bing Maps Aerial with Labels imagery",
          "Cesium World Terrain",
          1,
          "Bing Maps Aerial with Labels",
          3}),
      MakeShared<QuickAddItem>(QuickAddItem{
          "Cesium World Terrain + Bing Maps Road imagery",
          "Cesium World Terrain",
          1,
          "Bing Maps Road",
          4}),
      MakeShared<QuickAddItem>(QuickAddItem{
          "Cesium World Terrain + Sentinel-2 imagery",
          "Cesium World Terrain",
          1,
          "Sentinel-2 imagery",
          3954}),
      MakeShared<QuickAddItem>(QuickAddItem{
          "Cesium OSM Buildings",
          "Cesium OSM Buildings",
          96188,
          "",
          -1})};

  return SNew(SListView<TSharedRef<QuickAddItem>>)
      .SelectionMode(ESelectionMode::None)
      .ListItemsSource(&quickAddItems)
      .OnMouseButtonDoubleClick(this, &IonQuickAddPanel::AddItemToLevel)
      .OnGenerateRow(this, &IonQuickAddPanel::CreateQuickAddItemRow);
}

TSharedRef<ITableRow> IonQuickAddPanel::CreateQuickAddItemRow(
    TSharedRef<QuickAddItem> item,
    const TSharedRef<STableViewBase>& list) {
  return SNew(STableRow<TSharedRef<QuickAddItem>>, list)
      .Content()
          [SNew(SBox)
               .HAlign(EHorizontalAlignment::HAlign_Fill)
               .HeightOverride(40.0f)
               .Content()
                   [SNew(SHorizontalBox) +
                    SHorizontalBox::Slot().FillWidth(1.0f).Padding(5.0f).VAlign(
                        EVerticalAlignment::VAlign_Center)
                        [SNew(STextBlock)
                             .AutoWrapText(true)
                             .Text(
                                 FText::FromString(utf8_to_wstr(item->name)))] +
                    SHorizontalBox::Slot().AutoWidth().VAlign(
                        EVerticalAlignment::VAlign_Center)
                        [PropertyCustomizationHelpers::MakeNewBlueprintButton(
                            FSimpleDelegate::CreateLambda(
                                [this, item]() { this->AddItemToLevel(item); }),
                            FText::FromString(
                                TEXT("Add this dataset to the level")))]]];
}

void IonQuickAddPanel::AddItemToLevel(TSharedRef<QuickAddItem> item) {
  ACesium3DTileset* pTileset =
      FCesiumEditorModule::FindFirstTilesetWithAssetID(item->tilesetID);
  if (!pTileset) {
    pTileset =
        FCesiumEditorModule::CreateTileset(item->tilesetName, item->tilesetID);
  }

  if (item->overlayID > 0) {
    FCesiumEditorModule::AddOverlay(
        pTileset,
        item->overlayName,
        item->overlayID);
  }

  pTileset->RerunConstructionScripts();

  GEditor->SelectNone(true, false);
  GEditor->SelectActor(pTileset, true, true, true, true);
}