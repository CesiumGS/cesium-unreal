// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SHeaderRow.h"
#include "CesiumIonClient/Assets.h"

class FArguments;
class ITableRow;
class STableViewBase;

template <typename ItemType>
class SListView;

class CesiumIonPanel : public SCompoundWidget {
    SLATE_BEGIN_ARGS(CesiumIonPanel)
    {}
    SLATE_END_ARGS()

    CesiumIonPanel();
    virtual ~CesiumIonPanel();

    void Construct(const FArguments& InArgs);

    void Refresh();

private:
    TSharedRef<SWidget> AssetDetails();
    TSharedRef<ITableRow> CreateAssetRow(TSharedPtr<CesiumIonClient::Asset> item, const TSharedRef<STableViewBase>& list);
    void AddAssetToLevel(TSharedPtr<CesiumIonClient::Asset> item);
    void AddOverlayToTerrain(TSharedPtr<CesiumIonClient::Asset> item);
    void AddAsset(TSharedPtr<CesiumIonClient::Asset> item);
    void AssetSelected(TSharedPtr<CesiumIonClient::Asset> item, ESelectInfo::Type selectionType);

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
    void OnSortChange(EColumnSortPriority::Type SortPriority, const FName& ColumnName, EColumnSortMode::Type NewSortMode);

    FDelegateHandle _connectionUpdatedDelegateHandle;
    FDelegateHandle _assetsUpdatedDelegateHandle;
    TSharedPtr<SListView<TSharedPtr<CesiumIonClient::Asset>>> _pListView;
    TArray<TSharedPtr<CesiumIonClient::Asset>> _assets;
    bool _refreshInProgress;
    bool _refreshNeeded;
    TSharedPtr<SWidget> _pDetails;
    TSharedPtr<CesiumIonClient::Asset> _pSelection;

    /** 
     * The column name based on which the main assets list view is currently sorted.
     */
    FName _sortColumnName;

    /**
     * The sort mode that is currently applied to the _sortColumnName.
     */
    EColumnSortMode::Type _sortMode = EColumnSortMode::Type::None;

};
