// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumIonPanel.h"
#include "Cesium3DTilesSelection/Tile.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "Cesium3DTileset.h"
#include "CesiumCommands.h"
#include "CesiumEditor.h"
#include "CesiumIonRasterOverlay.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "EngineUtils.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IonLoginPanel.h"
#include "IonQuickAddPanel.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SHeader.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Views/SListView.h"

using namespace CesiumIonClient;

// Identifiers for the columns of the asset table view
static FName ColumnName_Name = "Name";
static FName ColumnName_Type = "Type";
static FName ColumnName_DateAdded = "DateAdded";

CesiumIonPanel::CesiumIonPanel()
    : _connectionUpdatedDelegateHandle(),
      _assetsUpdatedDelegateHandle(),
      _pListView(nullptr),
      _assets(),
      _pSelection(nullptr) {
  this->_connectionUpdatedDelegateHandle =
      FCesiumEditorModule::ion().ConnectionUpdated.AddRaw(
          this,
          &CesiumIonPanel::Refresh);
  this->_assetsUpdatedDelegateHandle =
      FCesiumEditorModule::ion().AssetsUpdated.AddRaw(
          this,
          &CesiumIonPanel::Refresh);
  this->_sortColumnName = ColumnName_DateAdded;
  this->_sortMode = EColumnSortMode::Type::Descending;
}

CesiumIonPanel::~CesiumIonPanel() {
  FCesiumEditorModule::ion().AssetsUpdated.Remove(
      this->_assetsUpdatedDelegateHandle);
  FCesiumEditorModule::ion().ConnectionUpdated.Remove(
      this->_connectionUpdatedDelegateHandle);
}

void CesiumIonPanel::Construct(const FArguments& InArgs) {
  // A function that returns the lambda that is used for rendering
  // the sort mode indicator of the header column: If sorting is
  // currently done based on the given name, then this will
  // return the current _sortMode. Otherwise, it will return
  // the 'None' sort mode.
  auto sortModeLambda = [this](const FName& columnName) {
    return [this, columnName]() {
      if (_sortColumnName != columnName) {
        return EColumnSortMode::None;
      }
      return _sortMode;
    };
  };

  this->_pListView =
      SNew(SListView<TSharedPtr<Asset>>)
          .ListItemsSource(&this->_assets)
          .OnMouseButtonDoubleClick(this, &CesiumIonPanel::AddAsset)
          .OnGenerateRow(this, &CesiumIonPanel::CreateAssetRow)
          .OnSelectionChanged(this, &CesiumIonPanel::AssetSelected)
          .HeaderRow(
              SNew(SHeaderRow) +
              SHeaderRow::Column(ColumnName_Name)
                  .DefaultLabel(FText::FromString(TEXT("Name")))
                  .SortMode_Lambda(sortModeLambda(ColumnName_Name))
                  .OnSort(FOnSortModeChanged::CreateSP(
                      this,
                      &CesiumIonPanel::OnSortChange)) +
              SHeaderRow::Column(ColumnName_Type)
                  .DefaultLabel(FText::FromString(TEXT("Type")))
                  .SortMode_Lambda(sortModeLambda(ColumnName_Type))
                  .OnSort(FOnSortModeChanged::CreateSP(
                      this,
                      &CesiumIonPanel::OnSortChange)) +
              SHeaderRow::Column(ColumnName_DateAdded)
                  .DefaultLabel(FText::FromString(TEXT("Date added")))
                  .SortMode_Lambda(sortModeLambda(ColumnName_DateAdded))
                  .OnSort(FOnSortModeChanged::CreateSP(
                      this,
                      &CesiumIonPanel::OnSortChange)));

  TSharedPtr<SWidget> pDetails = this->AssetDetails();

  // Create a splitter where the left shows the actual asset list
  // (with the controls (search, refresh) on top), and the right
  // shows the AssetDetails panel

  // clang-format off
  ChildSlot[
    SNew(SSplitter).Orientation(EOrientation::Orient_Horizontal) +
      SSplitter::Slot().Value(0.66f)
        [
          SNew(SVerticalBox) +
            SVerticalBox::Slot().AutoHeight()
            [
              SNew(SHorizontalBox) +
                // Add the refresh button at the upper left
                SHorizontalBox::Slot().HAlign(HAlign_Left).Padding(5.0f)
                [
                  SNew(SButton)
                    .HAlign(EHorizontalAlignment::HAlign_Center)
                    .Text(FText::FromString(TEXT("Refresh")))
                    .ToolTipText(FText::FromString(TEXT("Refresh the asset list")))
                    .OnClicked_Lambda([this]() {
                      FCesiumEditorModule::ion().refreshAssets();
                      Refresh();
                      return FReply::Handled();
                    })
                    [
                      SNew(SImage).Image(
                        FCesiumEditorModule::GetStyle()->GetBrush(
                          TEXT("Cesium.Common.Refresh")))
                    ]
                ] +
                // Add the search bar at the upper right
                SHorizontalBox::Slot().HAlign(HAlign_Right).Padding(5.0f)
                [
                  SAssignNew(SearchBox, SSearchBox).OnTextChanged(
                      this, &CesiumIonPanel::OnSearchTextChange)
                    .MinDesiredWidth(200.f)
                ]
            ] +
            SVerticalBox::Slot()
            [
              this->_pListView.ToSharedRef()
            ]
          ] +
          SSplitter::Slot().Value(0.34f)
          [
            SNew(SBorder).Padding(10)
            [
              pDetails.ToSharedRef()
            ]
          ]
        ];
  // clang-format on

  FCesiumEditorModule::ion().refreshAssets();
}

void CesiumIonPanel::OnSortChange(
    const EColumnSortPriority::Type SortPriority,
    const FName& ColumnName,
    const EColumnSortMode::Type Mode) {
  if (_sortColumnName == ColumnName) {
    if (_sortMode == EColumnSortMode::Type::None) {
      _sortMode = EColumnSortMode::Type::Ascending;
    } else if (_sortMode == EColumnSortMode::Type::Ascending) {
      _sortMode = EColumnSortMode::Type::Descending;
    } else {
      _sortMode = EColumnSortMode::Type::None;
    }
  } else {
    _sortColumnName = ColumnName;
    _sortMode = EColumnSortMode::Type::Ascending;
  }
  Refresh();
}

void CesiumIonPanel::OnSearchTextChange(const FText& SearchText) {
  _searchString = SearchText.ToString().TrimStartAndEnd();
  Refresh();
}

static bool isSupportedTileset(const TSharedPtr<Asset>& pAsset) {
  return pAsset && (pAsset->type == "3DTILES" || pAsset->type == "TERRAIN");
}

static bool isSupportedImagery(const TSharedPtr<Asset>& pAsset) {
  return pAsset && pAsset->type == "IMAGERY";
}

TSharedRef<SWidget> CesiumIonPanel::AssetDetails() {
  return SNew(SScrollBox).Visibility_Lambda([this]() {
    return this->_pSelection ? EVisibility::Visible : EVisibility::Collapsed;
  }) +
         SScrollBox::Slot().Padding(
             10,
             10,
             10,
             0)[SNew(STextBlock)
                    .AutoWrapText(true)
                    .TextStyle(FCesiumEditorModule::GetStyle(), "Heading")
                    .Text_Lambda([this]() {
                      return FText::FromString(
                          UTF8_TO_TCHAR(this->_pSelection->name.c_str()));
                    })] +
         SScrollBox::Slot()
             .Padding(10, 5, 10, 10)
             .HAlign(EHorizontalAlignment::HAlign_Fill)
                 [SNew(STextBlock).Text_Lambda([this]() {
                   return FText::FromString(UTF8_TO_TCHAR(
                       (std::string("(ID: ") +
                        std::to_string(this->_pSelection->id) + ")")
                           .c_str()));
                 })] +
         SScrollBox::Slot().Padding(10).HAlign(
             EHorizontalAlignment::HAlign_Fill)
             [SNew(SButton)
                  .Visibility_Lambda([this]() {
                    return isSupportedTileset(this->_pSelection)
                               ? EVisibility::Visible
                               : EVisibility::Collapsed;
                  })
                  .HAlign(EHorizontalAlignment::HAlign_Center)
                  .Text(FText::FromString(TEXT("Add to Level")))
                  .OnClicked_Lambda([this]() {
                    this->AddAssetToLevel(this->_pSelection);
                    return FReply::Handled();
                  })] +
         SScrollBox::Slot().Padding(10).HAlign(
             EHorizontalAlignment::HAlign_Fill)
             [SNew(SButton)
                  .Visibility_Lambda([this]() {
                    return isSupportedImagery(this->_pSelection)
                               ? EVisibility::Visible
                               : EVisibility::Collapsed;
                  })
                  .HAlign(EHorizontalAlignment::HAlign_Center)
                  .Text(FText::FromString(TEXT("Drape Over Terrain Tileset")))
                  .OnClicked_Lambda([this]() {
                    this->AddOverlayToTerrain(this->_pSelection);
                    return FReply::Handled();
                  })] +
         SScrollBox::Slot().Padding(10).HAlign(
             EHorizontalAlignment::HAlign_Fill)
             [SNew(SButton)
                  .Visibility_Lambda([this]() {
                    return !isSupportedTileset(this->_pSelection) &&
                                   !isSupportedImagery(this->_pSelection)
                               ? EVisibility::Visible
                               : EVisibility::Collapsed;
                  })
                  .HAlign(EHorizontalAlignment::HAlign_Center)
                  .Text(FText::FromString(
                      TEXT("This type of asset is not currently supported")))
                  .IsEnabled(false)] +
         SScrollBox::Slot().Padding(10).HAlign(
             EHorizontalAlignment::HAlign_Fill)
             [SNew(STextBlock)
                  .TextStyle(
                      FCesiumEditorModule::GetStyle(),
                      "AssetDetailsFieldHeader")
                  .Text(FText::FromString(TEXT("Description")))] +
         SScrollBox::Slot().Padding(
             10,
             0)[SNew(STextBlock)
                    .AutoWrapText(true)
                    .TextStyle(
                        FCesiumEditorModule::GetStyle(),
                        "AssetDetailsFieldValue")
                    .Text_Lambda([this]() {
                      return FText::FromString(UTF8_TO_TCHAR(
                          this->_pSelection->description.c_str()));
                    })] +
         SScrollBox::Slot().Padding(10).HAlign(
             EHorizontalAlignment::HAlign_Fill)
             [SNew(STextBlock)
                  .TextStyle(
                      FCesiumEditorModule::GetStyle(),
                      "AssetDetailsFieldHeader")
                  .Text(FText::FromString(TEXT("Attribution")))] +
         SScrollBox::Slot().Padding(
             10,
             0)[SNew(STextBlock)
                    .AutoWrapText(true)
                    .TextStyle(
                        FCesiumEditorModule::GetStyle(),
                        "AssetDetailsFieldValue")
                    .Text_Lambda([this]() {
                      return FText::FromString(UTF8_TO_TCHAR(
                          this->_pSelection->attribution.c_str()));
                    })];
}

/**
 * @brief Returns a comparator for the property of an Asset that is
 * associated with the given column name.
 *
 * @param columnName The column name
 * @return The comparator, comparing is ascending order (comparing by
 * the asset->name by default, if the given column name was not known)
 */
static std::function<bool(const TSharedPtr<Asset>&, const TSharedPtr<Asset>&)>
comparatorFor(const FName& columnName) {
  if (columnName == ColumnName_Type) {
    return [](const TSharedPtr<Asset>& a0, const TSharedPtr<Asset>& a1) {
      return a0->type < a1->type;
    };
  }
  if (columnName == ColumnName_DateAdded) {
    return [](const TSharedPtr<Asset>& a0, const TSharedPtr<Asset>& a1) {
      return a0->dateAdded < a1->dateAdded;
    };
  }
  return [](const TSharedPtr<Asset>& a0, const TSharedPtr<Asset>& a1) {
    return a0->name < a1->name;
  };
}

void CesiumIonPanel::ApplyFilter() {

  // UE_LOG(LogCesiumEditor, Warning, TEXT("ApplyFilter %s"), *_searchString);

  if (_searchString.IsEmpty()) {
    return;
  }
  this->_assets =
      this->_assets.FilterByPredicate([this](const TSharedPtr<Asset>& Asset) {
        // This mimics the behavior of the ion web UI, which
        // searches for the given text in the name and description.
        //
        // Creating and using FString instances here instead of
        // converting the _searchString to a std::string, because
        // the 'FString::Contains' function does the desired
        // case-INsensitive check by default.
        FString Name = UTF8_TO_TCHAR(Asset->name.c_str());
        if (Name.Contains(_searchString)) {
          return true;
        }
        FString Description = UTF8_TO_TCHAR(Asset->description.c_str());
        if (Description.Contains(_searchString)) {
          return true;
        }
        return false;
      });
}

void CesiumIonPanel::ApplySorting() {

  // UE_LOG(LogCesiumEditor, Warning, TEXT("ApplySorting %s with %d"),
  // *_sortColumnName.ToString(), _sortMode);

  if (_sortMode == EColumnSortMode::Type::None) {
    return;
  }
  auto baseComparator = comparatorFor(_sortColumnName);
  if (_sortMode == EColumnSortMode::Type::Ascending) {
    this->_assets.Sort(baseComparator);
  } else {
    this->_assets.Sort(
        [&baseComparator](
            const TSharedPtr<Asset>& a0,
            const TSharedPtr<Asset>& a1) { return baseComparator(a1, a0); });
  }
}

void CesiumIonPanel::Refresh() {
  const Assets& assets = FCesiumEditorModule::ion().getAssets();

  this->_assets.SetNum(assets.items.size());

  for (size_t i = 0; i < assets.items.size(); ++i) {
    this->_assets[i] = MakeShared<Asset>(assets.items[i]);
  }
  ApplyFilter();
  ApplySorting();
  this->_pListView->RequestListRefresh();
}

void CesiumIonPanel::Tick(
    const FGeometry& AllottedGeometry,
    const double InCurrentTime,
    const float InDeltaTime) {
  FCesiumEditorModule::ion().getAsyncSystem().dispatchMainThreadTasks();
  SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

void CesiumIonPanel::AssetSelected(
    TSharedPtr<CesiumIonClient::Asset> item,
    ESelectInfo::Type selectionType) {
  this->_pSelection = item;
}

void CesiumIonPanel::AddAsset(TSharedPtr<CesiumIonClient::Asset> item) {

  if (isSupportedImagery(item)) {
    this->AddOverlayToTerrain(item);
  } else if (isSupportedTileset(item)) {
    this->AddAssetToLevel(item);
  } else {
    UE_LOG(
        LogCesiumEditor,
        Warning,
        TEXT("Cannot add asset of type %s"),
        item->type.c_str());
  }
}

void CesiumIonPanel::AddAssetToLevel(TSharedPtr<CesiumIonClient::Asset> item) {
  UWorld* pCurrentWorld = GEditor->GetEditorWorldContext().World();
  ULevel* pCurrentLevel = pCurrentWorld->GetCurrentLevel();

  AActor* pNewActor = GEditor->AddActor(
      pCurrentLevel,
      ACesium3DTileset::StaticClass(),
      FTransform(),
      false,
      RF_Public | RF_Transactional);
  ACesium3DTileset* pTileset = Cast<ACesium3DTileset>(pNewActor);
  pTileset->SetActorLabel(UTF8_TO_TCHAR(item->name.c_str()));
  pTileset->SetIonAssetID(item->id);
  pTileset->SetIonAccessToken(UTF8_TO_TCHAR(
      FCesiumEditorModule::ion().getAssetAccessToken().token.c_str()));

  pTileset->RerunConstructionScripts();
}

void CesiumIonPanel::AddOverlayToTerrain(
    TSharedPtr<CesiumIonClient::Asset> item) {
  UWorld* pCurrentWorld = GEditor->GetEditorWorldContext().World();
  ULevel* pCurrentLevel = pCurrentWorld->GetCurrentLevel();

  ACesium3DTileset* pTilesetActor =
      FCesiumEditorModule::FindFirstTilesetSupportingOverlays();
  if (!pTilesetActor) {
    pTilesetActor =
        FCesiumEditorModule::CreateTileset("Cesium World Terrain", 1);
  }

  UCesiumRasterOverlay* pOverlay =
      FCesiumEditorModule::AddOverlay(pTilesetActor, item->name, item->id);

  pTilesetActor->RerunConstructionScripts();

  GEditor->SelectNone(true, false);
  GEditor->SelectActor(pTilesetActor, true, true, true, true);
  GEditor->SelectComponent(pOverlay, true, true, true);
}

namespace {

/**
 * @brief Returns a short string indicating the given asset type.
 *
 * The input must be one of the strings indicating the type of
 * an asset, as of https://cesium.com/docs/rest-api/#tag/Assets.
 *
 * If the input is not a known type, then an unspecified error
 * indicator will be returned.
 *
 * @param assetType The asset type.
 * @return The string.
 */
std::string assetTypeToString(const std::string& assetType) {
  static std::map<std::string, std::string> lookup = {
      {"3DTILES", "3D Tiles"},
      {"GLTF", "glTF"},
      {"IMAGERY", "Imagery"},
      {"TERRAIN", "Terrain"},
      {"CZML", "CZML"},
      {"KML", "KML"},
      {"GEOJSON", "GeoJSON"}};
  auto it = lookup.find(assetType);
  if (it != lookup.end()) {
    return it->second;
  }
  return "(Unknown)";
}

/**
 * @brief Format the given asset date into a date string.
 *
 * The given string is assumed to be in ISO8601 format, as returned
 * from the `asset.dateAdded`. It will be returned as a string in
 * the YYYY-MM-DD format. If the string cannot be parsed, it will
 * be returned as-it-is.
 *
 * @param assetDate The asset date
 * @return The formatted string
 */
FString formatDate(const std::string& assetDate) {
  FString unrealDateString = UTF8_TO_TCHAR(assetDate.c_str());
  FDateTime dateTime;
  bool success = FDateTime::ParseIso8601(*unrealDateString, dateTime);
  if (!success) {
    UE_LOG(
        LogCesiumEditor,
        Warning,
        TEXT("Could not parse date %s"),
        assetDate.c_str());
    return UTF8_TO_TCHAR(assetDate.c_str());
  }
  return dateTime.ToString(TEXT("%Y-%m-%d"));
}

class AssetsTableRow : public SMultiColumnTableRow<TSharedPtr<Asset>> {
public:
  void Construct(
      const FArguments& InArgs,
      const TSharedRef<STableViewBase>& InOwnerTableView,
      const TSharedPtr<Asset>& pItem) {
    this->_pItem = pItem;
    SMultiColumnTableRow<TSharedPtr<Asset>>::Construct(
        InArgs,
        InOwnerTableView);
  }

  virtual TSharedRef<SWidget>
  GenerateWidgetForColumn(const FName& InColumnName) override {
    if (InColumnName == ColumnName_Name) {
      return SNew(STextBlock)
          .Text(FText::FromString(UTF8_TO_TCHAR(_pItem->name.c_str())));
    } else if (InColumnName == ColumnName_Type) {
      return SNew(STextBlock)
          .Text(FText::FromString(
              UTF8_TO_TCHAR(assetTypeToString(_pItem->type).c_str())));
    } else if (InColumnName == ColumnName_DateAdded) {
      return SNew(STextBlock)
          .Text(FText::FromString(formatDate(_pItem->dateAdded)));
    } else {
      return SNew(STextBlock);
    }
  }

private:
  TSharedPtr<Asset> _pItem;
};

} // namespace

TSharedRef<ITableRow> CesiumIonPanel::CreateAssetRow(
    TSharedPtr<Asset> item,
    const TSharedRef<STableViewBase>& list) {
  return SNew(AssetsTableRow, list, item);
}
