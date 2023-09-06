// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumCustomization.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

IDetailGroup& CesiumCustomization::CreateGroup(
    IDetailCategoryBuilder& Category,
    FName GroupName,
    const FText& LocalizedDisplayName,
    bool bForAdvanced,
    bool bStartExpanded) {
  IDetailGroup& Group = Category.AddGroup(
      GroupName,
      LocalizedDisplayName,
      bForAdvanced,
      bStartExpanded);
  Group.HeaderRow()
      .WholeRowContent()
      .HAlign(HAlign_Left)
      .VAlign(VAlign_Center)
          [SNew(SButton)
               .ButtonStyle(FAppStyle::Get(), "NoBorder")
               .ContentPadding(FMargin(0, 2, 0, 2))
               .OnClicked_Lambda([&Group]() {
                 Group.ToggleExpansion(!Group.GetExpansionState());
                 return FReply::Handled();
               })
               .ForegroundColor(FSlateColor::UseForeground())
               .Content()[SNew(STextBlock)
                              .Font(IDetailLayoutBuilder::GetDetailFont())
                              .Text(LocalizedDisplayName)]

  ];
  return Group;
}
