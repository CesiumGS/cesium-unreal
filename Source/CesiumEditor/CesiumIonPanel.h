// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
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

    FDelegateHandle _connectionUpdatedDelegateHandle;
    FDelegateHandle _assetsUpdatedDelegateHandle;
    TSharedPtr<SListView<TSharedPtr<CesiumIonClient::Asset>>> _pListView;
    TArray<TSharedPtr<CesiumIonClient::Asset>> _assets;
    bool _refreshInProgress;
    bool _refreshNeeded;
    TSharedPtr<SWidget> _pDetails;
    TSharedPtr<CesiumIonClient::Asset> _pSelection;
};
