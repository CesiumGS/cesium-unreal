#pragma once

#include "Widgets/SCompoundWidget.h"
#include "CesiumIonClient/CesiumIonAssets.h"

class FArguments;
class ITableRow;
class STableViewBase;

template <typename ItemType>
class SListView;

class CesiumIonPanel : public SCompoundWidget {
    SLATE_BEGIN_ARGS(CesiumIonPanel)
    {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime);

private:
    void Refresh();
    TSharedRef<ITableRow> CreateAssetRow(TSharedPtr<CesiumIonClient::CesiumIonAsset> item, const TSharedRef<STableViewBase>& list);
    void AddAssetToLevel(TSharedPtr<CesiumIonClient::CesiumIonAsset> item);

    TSharedPtr<SListView<TSharedPtr<CesiumIonClient::CesiumIonAsset>>> _pListView;
    TArray<TSharedPtr<CesiumIonClient::CesiumIonAsset>> _assets;
    bool _refreshInProgress = false;
    bool _refreshNeeded = false;
};
