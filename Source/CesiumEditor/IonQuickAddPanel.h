#pragma once

#include "Widgets/SCompoundWidget.h"

class FArguments;

class IonQuickAddPanel : public SCompoundWidget {
    SLATE_BEGIN_ARGS(IonQuickAddPanel)
    {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    struct QuickAddItem {
        std::string name{};
        int64_t tilesetID = -1;
        int64_t overlayID = -1;
    };

    TSharedRef<SWidget> QuickAddList();
    TSharedRef<ITableRow> CreateQuickAddItemRow(TSharedRef<QuickAddItem> item, const TSharedRef<STableViewBase>& list);
    void AddItemToLevel(TSharedRef<QuickAddItem> item);
};
