// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumIonClient/Assets.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SHeaderRow.h"

class FArguments;
class ITableRow;
class STableViewBase;

template <typename ItemType> class SListView;

class CesiumIonPanel : public SCompoundWidget {
  SLATE_BEGIN_ARGS(CesiumIonPanel) {}
  SLATE_END_ARGS()

  CesiumIonPanel();
  virtual ~CesiumIonPanel();

  void Construct(const FArguments& InArgs);

  void Refresh();

  virtual void Tick(
      const FGeometry& AllottedGeometry,
      const double InCurrentTime,
      const float InDeltaTime) override;

private:
  TSharedRef<SWidget> AssetDetails();
  TSharedRef<ITableRow> CreateAssetRow(
      TSharedPtr<CesiumIonClient::Asset> item,
      const TSharedRef<STableViewBase>& list);
  void AddAssetToLevel(TSharedPtr<CesiumIonClient::Asset> item);
  void AddOverlayToTerrain(
      TSharedPtr<CesiumIonClient::Asset> item,
      bool useAsBaseLayer);
  void AddAsset(TSharedPtr<CesiumIonClient::Asset> item);
  void AssetSelected(
      TSharedPtr<CesiumIonClient::Asset> item,
      ESelectInfo::Type selectionType);

  /**
   * Filter the current _assets array, based on the current _searchString.
   * This will replace the _assets array with one that only contains
   * assets whose name or description contain the search string
   */
  void ApplyFilter();

  /**
   * Sort the current _assets array, based on the current _sortColumnName
   * and _sortMode, before using it to populate the list view.
   */
  void ApplySorting();

  /**
   * Will be called whenever one header of the asset list view is
   * clicked, and update the current _sortColumnName and _sortMode
   * accordingly.
   */
  void OnSortChange(
      EColumnSortPriority::Type SortPriority,
      const FName& ColumnName,
      EColumnSortMode::Type NewSortMode);

  /**
   * Will be called whenever the contents of the _SearchBox changes,
   * store the corresponding _searchString, and refresh the view.
   */
  void OnSearchTextChange(const FText& SearchText);

  FDelegateHandle _connectionUpdatedDelegateHandle;
  FDelegateHandle _assetsUpdatedDelegateHandle;
  FDelegateHandle _assetAccessTokenUpdatedDelegateHandle;
  TSharedPtr<SListView<TSharedPtr<CesiumIonClient::Asset>>> _pListView;
  TArray<TSharedPtr<CesiumIonClient::Asset>> _assets;
  TSharedPtr<CesiumIonClient::Asset> _pSelection;

  /**
   * The column name based on which the main assets list view is currently
   * sorted.
   */
  FName _sortColumnName;

  /**
   * The sort mode that is currently applied to the _sortColumnName.
   */
  EColumnSortMode::Type _sortMode = EColumnSortMode::Type::None;

  /**
   * The search box for entering the _searchString
   */
  TSharedPtr<SSearchBox> SearchBox;

  /**
   * The string that is currently entered in the SearchBox,
   * (trimmed from whitespace), used for filtering the asset
   * list in ApplyFilter.
   */
  FString _searchString;
};
