// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Dialogs/CustomDialog.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STableRow.h"
#include <string>
#include <unordered_set>

class FArguments;

class IonQuickAddPanel : public SCompoundWidget {
  SLATE_BEGIN_ARGS(IonQuickAddPanel) {}
  SLATE_END_ARGS()

  void Construct(const FArguments& InArgs);

private:
  struct QuickAddItem {
    std::string name{};
    std::string tilesetName{};
    int64_t tilesetID = -1;
    std::string overlayName{};
    int64_t overlayID = -1;
  };

  TSharedRef<SWidget> QuickAddList();
  TSharedRef<ITableRow> CreateQuickAddItemRow(
      TSharedRef<QuickAddItem> item,
      const TSharedRef<STableViewBase>& list);
  void AddItemToLevel(TSharedRef<QuickAddItem> item);

  std::unordered_set<std::string> _itemsBeingAdded;
};
