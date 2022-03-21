// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Dialogs/CustomDialog.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STableRow.h"
#include <string>
#include <unordered_set>

class FArguments;

enum class QuickAddItemType { TILESET, SUNSKY, DYNAMIC_PAWN };

struct QuickAddItem {
  QuickAddItemType type;
  std::string name{};
  std::string description;
  std::string tilesetName{};
  int64_t tilesetID = -1;
  std::string overlayName{};
  int64_t overlayID = -1;
};

class IonQuickAddPanel : public SCompoundWidget {
  SLATE_BEGIN_ARGS(IonQuickAddPanel) {}
  /**
   * The tile shown over the elements of the list
   */
  SLATE_ARGUMENT(FText, Title)
  SLATE_END_ARGS()

  void Construct(const FArguments& InArgs);

  void AddItem(const QuickAddItem& item);

private:
  TSharedRef<SWidget> QuickAddList();
  TSharedRef<ITableRow> CreateQuickAddItemRow(
      TSharedRef<QuickAddItem> item,
      const TSharedRef<STableViewBase>& list);

  void AddItemToLevel(TSharedRef<QuickAddItem> item);
  void AddBlankTilesetToLevel();
  void AddIonTilesetToLevel(TSharedRef<QuickAddItem> item);
  void AddCesiumSunSkyToLevel();
  void AddDynamicPawnToLevel();

  TArray<TSharedRef<QuickAddItem>> _quickAddItems;
  std::unordered_set<std::string> _itemsBeingAdded;
};
