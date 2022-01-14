// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "IonQuickAddPanel.h"
#include "Cesium3DTileset.h"
#include "CesiumEditor.h"
#include "CesiumIonClient/Connection.h"
#include "CesiumIonRasterOverlay.h"
#include "Editor.h"
#include "PropertyCustomizationHelpers.h"
#include "Styling/SlateStyle.h"
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
           .AutoHeight()
           .Padding(FMargin(5.0f, 20.0f, 5.0f, 10.0f))
               [SNew(SHeader).Content()
                    [SNew(STextBlock)
                         .TextStyle(FCesiumEditorModule::GetStyle(), "Heading")
                         .Text(InArgs._Title)]] +
       SVerticalBox::Slot()
           .VAlign(VAlign_Top)
           .Padding(FMargin(5.0f, 0.0f, 5.0f, 20.0f))[this->QuickAddList()]];
}

void IonQuickAddPanel::AddItem(const QuickAddItem& item) {
  _quickAddItems.Add(MakeShared<QuickAddItem>(item));
}

TSharedRef<SWidget> IonQuickAddPanel::QuickAddList() {
  return SNew(SListView<TSharedRef<QuickAddItem>>)
      .SelectionMode(ESelectionMode::None)
      .ListItemsSource(&_quickAddItems)
      .OnMouseButtonDoubleClick(this, &IonQuickAddPanel::AddItemToLevel)
      .OnGenerateRow(this, &IonQuickAddPanel::CreateQuickAddItemRow);
}

TSharedRef<ITableRow> IonQuickAddPanel::CreateQuickAddItemRow(
    TSharedRef<QuickAddItem> item,
    const TSharedRef<STableViewBase>& list) {
  // clang-format off
  return SNew(STableRow<TSharedRef<QuickAddItem>>, list).Content()
  [
    SNew(SBox)
      .HAlign(EHorizontalAlignment::HAlign_Fill)
      .HeightOverride(40.0f)
      .Content()
      [
        SNew(SHorizontalBox) + 
          SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .Padding(5.0f)
            .VAlign(EVerticalAlignment::VAlign_Center)
            [
              SNew(STextBlock)
                .AutoWrapText(true)
                .Text(FText::FromString(UTF8_TO_TCHAR(item->name.c_str())))
                .ToolTipText(FText::FromString(UTF8_TO_TCHAR(item->description.c_str())))
            ] +
          SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(EVerticalAlignment::VAlign_Center)
            [
              PropertyCustomizationHelpers::MakeNewBlueprintButton(
                FSimpleDelegate::CreateLambda(
                  [this, item]() { this->AddItemToLevel(item); }),
                FText::FromString(
                  TEXT("Add this item to the level")),
                  TAttribute<bool>::Create([this, item]() {
                    return this->_itemsBeingAdded.find(item->name) ==
                            this->_itemsBeingAdded.end();
                  })
              )
            ]
      ]
  ];
  // clang-format on
}

namespace {

void showAssetDepotConfirmWindow(
    const FString& itemName,
    int64_t missingAsset) {

  // clang-format off
  TSharedRef<SWindow> AssetDepotConfirmWindow =
    SNew(SWindow)
      .Title(FText::FromString(TEXT("Asset is not available in My Assets")))
      .ClientSize(FVector2D(400.0f, 200.0f))
      .Content()
      [
        SNew(SVerticalBox) +
          SVerticalBox::Slot().AutoHeight().Padding(10.0f)
          [
            SNew(STextBlock)
              .AutoWrapText(true)
              .Text(FText::FromString(TEXT("Before " + itemName +
                " can be added to your level, it must be added to \"My Assets\" in your Cesium ion account.")))
          ] +
          SVerticalBox::Slot()
            .AutoHeight()
            .HAlign(EHorizontalAlignment::HAlign_Left)
            .Padding(10.0f, 5.0f)
          [
            SNew(SHyperlink)
              .OnNavigate_Lambda([missingAsset]() {
                FPlatformProcess::LaunchURL(
                    UTF8_TO_TCHAR(
                        ("https://cesium.com/ion/assetdepot/" +
                          std::to_string(missingAsset))
                            .c_str()),
                    NULL,
                    NULL);
              })
              .Text(FText::FromString(TEXT(
                  "Open this asset in the Cesium ion Asset Depot")))
          ] +
          SVerticalBox::Slot()
            .AutoHeight()
            .HAlign(EHorizontalAlignment::HAlign_Left)
            .Padding(10.0f, 5.0f)
          [
            SNew(STextBlock).Text(FText::FromString(TEXT(
              "Click \"Add to my assets\" in the Cesium ion web page")))
          ] +
          SVerticalBox::Slot()
            .AutoHeight()
            .HAlign(EHorizontalAlignment::HAlign_Left)
            .Padding(10.0f, 5.0f)
          [
            SNew(STextBlock)
              .Text(FText::FromString(TEXT(
                "Return to Cesium for Unreal and try adding this asset again")))
          ] +
          SVerticalBox::Slot()
            .AutoHeight()
            .HAlign(EHorizontalAlignment::HAlign_Center)
            .Padding(10.0f, 25.0f)
          [
            SNew(SButton)
              .OnClicked_Lambda(
                  [&AssetDepotConfirmWindow]() {
                    AssetDepotConfirmWindow
                        ->RequestDestroyWindow();
                    return FReply::Handled();
                  })
              .Text(FText::FromString(TEXT("Close")))
          ]
      ];
  // clang-format on
  GEditor->EditorAddModalWindow(AssetDepotConfirmWindow);
}
} // namespace

void IonQuickAddPanel::AddIonTilesetToLevel(TSharedRef<QuickAddItem> item) {
  const std::optional<CesiumIonClient::Connection>& connection =
      FCesiumEditorModule::ion().getConnection();
  if (!connection) {
    UE_LOG(
        LogCesiumEditor,
        Warning,
        TEXT("Cannot add an ion asset without an active connection"));
    return;
  }
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
          FString itemName(UTF8_TO_TCHAR(item->name.c_str()));
          showAssetDepotConfirmWindow(itemName, missingAsset);
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
            FCesiumEditorModule::AddBaseOverlay(
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

void IonQuickAddPanel::AddCesiumSunSkyToLevel() {
  AActor* pActor = FCesiumEditorModule::GetCurrentLevelCesiumSunSky();
  if (!pActor) {
    pActor = FCesiumEditorModule::SpawnCesiumSunSky();
  }

  if (pActor) {
    GEditor->SelectNone(true, false);
    GEditor->SelectActor(pActor, true, true, true, true);
  }
}

namespace {
/**
 * Set a byte property value in the given object.
 *
 * This will search the class of the given object for a property
 * with the given name, which is assumed to be a byte property,
 * and assign the given value to this property.
 *
 * If the object is `nullptr`, nothing will be done.
 * If the class does not contain a property with the given name,
 * or it is not a byte property, a warning will be printed.
 *
 * @param object The object to set the value in
 * @param name The name of the property
 * @param value The value to set for the property.
 */
void SetBytePropertyValue(
    UObjectBase* object,
    const std::string& name,
    uint8 value) {
  if (!object) {
    return;
  }
  UClass* cls = object->GetClass();
  FString nameString(name.c_str());
  FName propName = FName(*nameString);
  FProperty* property = cls->FindPropertyByName(propName);
  if (!property) {
    UE_LOG(
        LogCesiumEditor,
        Warning,
        TEXT("Property with name %s not found"),
        *nameString);
    return;
  }
  FByteProperty* byteProperty = CastField<FByteProperty>(property);
  if (!byteProperty) {
    UE_LOG(
        LogCesiumEditor,
        Warning,
        TEXT("Property is not an FByteProperty: %s"),
        *nameString);
    return;
  }
  byteProperty->SetPropertyValue_InContainer(object, value);
}
} // namespace

void IonQuickAddPanel::AddDynamicPawnToLevel() {
  AActor* pActor = FCesiumEditorModule::GetCurrentLevelDynamicPawn();
  if (!pActor) {
    pActor = FCesiumEditorModule::SpawnDynamicPawn();
  }

  if (pActor) {
    uint8 autoPossessValue =
        static_cast<uint8>(EAutoReceiveInput::Type::Player0);
    SetBytePropertyValue(pActor, "AutoPossessPlayer", autoPossessValue);

    GEditor->SelectNone(true, false);
    GEditor->SelectActor(pActor, true, true, true, true);
  }
}

void IonQuickAddPanel::AddBlankTilesetToLevel() {
  UWorld* pCurrentWorld = GEditor->GetEditorWorldContext().World();
  ULevel* pCurrentLevel = pCurrentWorld->GetCurrentLevel();

  GEditor->AddActor(
      pCurrentLevel,
      ACesium3DTileset::StaticClass(),
      FTransform(),
      false,
      RF_Public | RF_Transactional);
}

void IonQuickAddPanel::AddItemToLevel(TSharedRef<QuickAddItem> item) {
  if (this->_itemsBeingAdded.find(item->name) != this->_itemsBeingAdded.end()) {
    // Add is already in progress.
    return;
  }
  this->_itemsBeingAdded.insert(item->name);

  if (item->type == QuickAddItemType::TILESET) {

    // The blank tileset (identified by the tileset and overlay ID being -1)
    // can be added directly. All ion tilesets are added via
    // AddIonTilesetToLevel, which requires an active connection.
    bool isBlankTileset = item->type == QuickAddItemType::TILESET &&
                          item->tilesetID == -1 && item->overlayID == -1;
    if (isBlankTileset) {
      AddBlankTilesetToLevel();
      this->_itemsBeingAdded.erase(item->name);
    } else {
      AddIonTilesetToLevel(item);
    }
  } else if (item->type == QuickAddItemType::SUNSKY) {
    AddCesiumSunSkyToLevel();
    this->_itemsBeingAdded.erase(item->name);
  } else if (item->type == QuickAddItemType::DYNAMIC_PAWN) {
    AddDynamicPawnToLevel();
    this->_itemsBeingAdded.erase(item->name);
  }
}
