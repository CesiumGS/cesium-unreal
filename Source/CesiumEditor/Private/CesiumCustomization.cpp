// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumCustomization.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Text/STextBlock.h"

struct CesiumButtonGroup::Impl {
  TSharedPtr<SWrapBox> Container;
  TArray<TWeakObjectPtr<UObject>> SelectedObjects;
  FTextBuilder ButtonSearchText;
};

CesiumButtonGroup::CesiumButtonGroup() : pImpl(MakeUnique<Impl>()) {
  this->pImpl->Container = SNew(SWrapBox).UseAllottedSize(true);
}

void CesiumButtonGroup::AddButtonForUFunction(
    UFunction* Function,
    const FText& Label) {
  check(Function);
  if (!Function)
    return;

  FText ButtonCaption =
      Label.IsEmpty()
          ? FText::FromString(
                FName::NameToDisplayString(*Function->GetName(), false))
          : Label;
  FText ButtonTooltip = Function->GetToolTipText();

  this->pImpl->ButtonSearchText.AppendLine(ButtonCaption);
  this->pImpl->ButtonSearchText.AppendLine(ButtonTooltip);

  TWeakObjectPtr<UFunction> WeakFunctionPtr = Function;

  pImpl->Container->AddSlot()
      .VAlign(EVerticalAlignment::VAlign_Center)
      .Padding(
          0.0f,
          3.0f,
          0.0f,
          3.0f)[SNew(SButton)
                    .Text(ButtonCaption)
                    .OnClicked_Lambda([WeakFunctionPtr,
                                       ButtonCaption,
                                       Group = this->AsShared()]() {
                      if (UFunction* Function = WeakFunctionPtr.Get()) {
                        FScopedTransaction Transaction(ButtonCaption);
                        FEditorScriptExecutionGuard ScriptGuard;
                        for (TWeakObjectPtr<UObject> SelectedObjectPtr :
                             Group->pImpl->SelectedObjects) {
                          if (UObject* Object = SelectedObjectPtr.Get()) {
                            Object->Modify();
                            Object->ProcessEvent(Function, nullptr);
                          }
                        }
                      }
                      return FReply::Handled();
                    })
                    .ToolTipText(ButtonTooltip)];
}

void CesiumButtonGroup::Finish(
    IDetailLayoutBuilder& DetailBuilder,
    IDetailCategoryBuilder& Category) {
  DetailBuilder.GetObjectsBeingCustomized(this->pImpl->SelectedObjects);
  Category.AddCustomRow(this->pImpl->ButtonSearchText.ToText())
      .RowTag("Actions")[this->pImpl->Container.ToSharedRef()];
}

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

TSharedPtr<CesiumButtonGroup> CesiumCustomization::CreateButtonGroup() {
  return MakeShared<CesiumButtonGroup>();
}
