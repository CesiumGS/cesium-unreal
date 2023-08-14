// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGeoreferenceCustomization.h"
#include "CesiumDegreesMinutesSecondsEditor.h"
#include "CesiumGeoreference.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditing.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<IDetailCustomization>
FCesiumGeoreferenceCustomization::MakeInstance() {
  return MakeShareable(new FCesiumGeoreferenceCustomization);
}

void FCesiumGeoreferenceCustomization::CustomizeDetails(
    IDetailLayoutBuilder& DetailBuilder) {
  DetailBuilder.GetObjectsBeingCustomized(this->SelectedObjects);

  IDetailCategoryBuilder& CesiumCategory = DetailBuilder.EditCategory("Cesium");

  UFunction* PlaceGeoreferenceOriginHere =
      DetailBuilder.GetBaseClass()->FindFunctionByName(
          GET_FUNCTION_NAME_CHECKED(
              ACesiumGeoreference,
              PlaceGeoreferenceOriginHere));
  check(PlaceGeoreferenceOriginHere);

  FText ButtonCaption = FText::FromString(FName::NameToDisplayString(
      *PlaceGeoreferenceOriginHere->GetName(),
      false));
  FText ButtonTooltip = PlaceGeoreferenceOriginHere->GetToolTipText();

  FTextBuilder ButtonSearchText;
  ButtonSearchText.AppendLine(ButtonCaption);
  ButtonSearchText.AppendLine(ButtonTooltip);

  TSharedPtr<SWrapBox> WrapBox = SNew(SWrapBox).UseAllottedSize(true);
  WrapBox->AddSlot().Padding(0.0f, 0.0f, 5.0f, 3.0f)
      [SNew(SButton)
           .Text(ButtonCaption)
           .OnClicked(FOnClicked::CreateSP(
               this,
               &FCesiumGeoreferenceCustomization::OnPlaceGeoreferenceOriginHere,
               TWeakObjectPtr<UFunction>(PlaceGeoreferenceOriginHere)))
           .ToolTipText(ButtonTooltip)];

  CesiumCategory.AddCustomRow(ButtonSearchText.ToText())
      .RowTag(PlaceGeoreferenceOriginHere->GetFName())[WrapBox.ToSharedRef()];

  CesiumCategory.AddProperty(
      GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, OriginPlacement));

  TSharedPtr<class IPropertyHandle> LongitudeDecimalDegreesHandle =
      DetailBuilder.GetProperty(
          GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, OriginLongitude));
  IDetailPropertyRow& LongitudeRow =
      CesiumCategory.AddProperty(LongitudeDecimalDegreesHandle);
  LongitudeEditor = MakeShared<CesiumDegreesMinutesSecondsEditor>(
      LongitudeDecimalDegreesHandle,
      true);
  LongitudeEditor->PopulateRow(LongitudeRow);

  TSharedPtr<class IPropertyHandle> LatitudeDecimalDegreesHandle =
      DetailBuilder.GetProperty(
          GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, OriginLatitude));
  IDetailPropertyRow& LatitudeRow =
      CesiumCategory.AddProperty(LatitudeDecimalDegreesHandle);
  LatitudeEditor = MakeShared<CesiumDegreesMinutesSecondsEditor>(
      LatitudeDecimalDegreesHandle,
      false);
  LatitudeEditor->PopulateRow(LatitudeRow);

  CesiumCategory.AddProperty(
      GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, OriginHeight));

  CesiumCategory.AddProperty(
      GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, Scale));
  CesiumCategory.AddProperty(
      GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, SubLevelCamera));
  CesiumCategory.AddProperty(
      GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, ShowLoadRadii));
}

FReply FCesiumGeoreferenceCustomization::OnPlaceGeoreferenceOriginHere(
    TWeakObjectPtr<UFunction> WeakFunctionPtr) {
  if (UFunction* Function = WeakFunctionPtr.Get()) {
    FScopedTransaction Transaction(
        FText::FromString("Place Georeference Origin Here"));

    FEditorScriptExecutionGuard ScriptGuard;
    for (TWeakObjectPtr<UObject> SelectedObjectPtr : SelectedObjects) {
      if (UObject* Object = SelectedObjectPtr.Get()) {
        Object->ProcessEvent(Function, nullptr);
      }
    }
  }

  return FReply::Handled();
}
