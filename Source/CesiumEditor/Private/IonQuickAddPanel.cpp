// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "IonQuickAddPanel.h"
#include "Cesium3DTileset.h"
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
#include "Widgets/Input/SHyperlink.h"
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
                                TEXT("Add this dataset to the level")),
                            TAttribute<bool>::Create([this, item]() {
                              return this->_itemsBeingAdded.find(item->name) ==
                                     this->_itemsBeingAdded.end();
                            }))]]];
}

void IonQuickAddPanel::AddItemToLevel(TSharedRef<QuickAddItem> item) {
  if (this->_itemsBeingAdded.find(item->name) != this->_itemsBeingAdded.end()) {
    // Add is already in progress.
    return;
  }

  const std::optional<CesiumIonClient::Connection>& connection =
      FCesiumEditorModule::ion().getConnection();
  if (!connection) {
    return;
  }

  this->_itemsBeingAdded.insert(item->name);

  connection->asset(item->tilesetID)
      .thenInMainThread([item, connection](Response<Asset>&& response) {
        if (!response.value.has_value()) {
          return connection->getAsyncSystem().createResolvedFuture<int64_t>(
              std::move(int64_t(item->tilesetID)));
        }

        if (item->overlayID >= 0) {
          return connection->asset(item->overlayID)
              .thenInMainThread([item](Response<Asset>&& overlayResponse) {
                return overlayResponse.value.has_value()
                           ? int64_t(-1)
                           : int64_t(item->overlayID);
              });
        } else {
          return connection->getAsyncSystem().createResolvedFuture<int64_t>(-1);
        }
      })
      .thenInMainThread([this, item](int64_t missingAsset) {
        if (missingAsset != -1) {
          TSharedRef<SWindow> AssetDepotConfirmWindow =
              SNew(SWindow)
                  .Title(FText::FromString(
                      TEXT("Asset is not available in My Assets")))
                  .ClientSize(FVector2D(400.0f, 200.0f))
                  .Content()
                      [SNew(SVerticalBox) +
                       SVerticalBox::Slot().AutoHeight().Padding(10.0f)
                           [SNew(STextBlock)
                                .AutoWrapText(true)
                                .Text(FText::FromString(TEXT(
                                    "Before " + utf8_to_wstr(item->name) +
                                    " can be added to your level, it must be added to \"My Assets\" in your Cesium ion account.")))] +
                       SVerticalBox::Slot()
                           .AutoHeight()
                           .HAlign(EHorizontalAlignment::HAlign_Left)
                           .Padding(10.0f, 5.0f)
                               [SNew(SHyperlink)
                                    .OnNavigate_Lambda([missingAsset]() {
                                      FPlatformProcess::LaunchURL(
                                          *utf8_to_wstr(
                                              "https://cesium.com/ion/assetdepot/" +
                                              std::to_string(missingAsset)),
                                          NULL,
                                          NULL);
                                    })
                                    .Text(FText::FromString(TEXT(
                                        "Open this asset in the Cesium ion Asset Depot")))] +
                       SVerticalBox::Slot()
                           .AutoHeight()
                           .HAlign(EHorizontalAlignment::HAlign_Left)
                           .Padding(10.0f, 5.0f)
                               [SNew(STextBlock)
                                    .Text(FText::FromString(TEXT(
                                        "Click \"Add to my assets\" in the Cesium ion web page")))] +
                       SVerticalBox::Slot()
                           .AutoHeight()
                           .HAlign(EHorizontalAlignment::HAlign_Left)
                           .Padding(10.0f, 5.0f)
                               [SNew(STextBlock)
                                    .Text(FText::FromString(TEXT(
                                        "Return to Cesium for Unreal and try adding this asset again")))] +
                       SVerticalBox::Slot()
                           .AutoHeight()
                           .HAlign(EHorizontalAlignment::HAlign_Center)
                           .Padding(10.0f, 25.0f)
                               [SNew(SButton)
                                    .OnClicked_Lambda(
                                        [&AssetDepotConfirmWindow]() {
                                          AssetDepotConfirmWindow
                                              ->RequestDestroyWindow();
                                          return FReply::Handled();
                                        })
                                    .Text(FText::FromString(TEXT("Close")))]];

          GEditor->EditorAddModalWindow(AssetDepotConfirmWindow);
        } else {
          ACesium3DTileset* pTileset =
              FCesiumEditorModule::FindFirstTilesetWithAssetID(item->tilesetID);
          if (!pTileset) {
            pTileset = FCesiumEditorModule::CreateTileset(
                item->tilesetName,
                item->tilesetID);
          }

          FCesiumEditorModule::ion().getAssets();

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

        this->_itemsBeingAdded.erase(item->name);
      });
}