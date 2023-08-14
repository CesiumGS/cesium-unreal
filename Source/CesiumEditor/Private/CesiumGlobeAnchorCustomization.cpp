// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumGlobeAnchorCustomization.h"
#include "CesiumDegreesMinutesSecondsEditor.h"
#include "CesiumGlobeAnchorComponent.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditing.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Text/STextBlock.h"

void UCesiumGlobeAnchorRotationEastSouthUp::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) {
  Super::PostEditChangeProperty(PropertyChangedEvent);
  this->Component->SetEastSouthUpRotation(
      FRotator(this->Pitch, this->Yaw, this->Roll).Quaternion());
  this->Component->Modify();
}

void FCesiumGlobeAnchorCustomization::Register(
    FPropertyEditorModule& PropertyEditorModule) {
  PropertyEditorModule.RegisterCustomClassLayout(
      UCesiumGlobeAnchorComponent::StaticClass()->GetFName(),
      FOnGetDetailCustomizationInstance::CreateStatic(
          &FCesiumGlobeAnchorCustomization::MakeInstance));
}

void FCesiumGlobeAnchorCustomization::Unregister(
    FPropertyEditorModule& PropertyEditorModule) {
  PropertyEditorModule.UnregisterCustomClassLayout(
      UCesiumGlobeAnchorComponent::StaticClass()->GetFName());
}

TSharedRef<IDetailCustomization>
FCesiumGlobeAnchorCustomization::MakeInstance() {
  return MakeShareable(new FCesiumGlobeAnchorCustomization);
}

void FCesiumGlobeAnchorCustomization::CustomizeDetails(
    IDetailLayoutBuilder& DetailBuilder) {
  DetailBuilder.GetObjectsBeingCustomized(this->SelectedObjects);

  IDetailCategoryBuilder& CesiumCategory = DetailBuilder.EditCategory("Cesium");

  // UFunction* PlaceGeoreferenceOriginHere =
  //     DetailBuilder.GetBaseClass()->FindFunctionByName(
  //         GET_FUNCTION_NAME_CHECKED(
  //             UCesiumGlobeAnchorComponent,
  //             PlaceGeoreferenceOriginHere));
  // check(PlaceGeoreferenceOriginHere);

  // FText ButtonCaption = FText::FromString(FName::NameToDisplayString(
  //     *PlaceGeoreferenceOriginHere->GetName(),
  //     false));
  // FText ButtonTooltip = PlaceGeoreferenceOriginHere->GetToolTipText();

  // FTextBuilder ButtonSearchText;
  // ButtonSearchText.AppendLine(ButtonCaption);
  // ButtonSearchText.AppendLine(ButtonTooltip);

  // TSharedPtr<SWrapBox> WrapBox = SNew(SWrapBox).UseAllottedSize(true);
  // WrapBox->AddSlot().Padding(0.0f, 0.0f, 5.0f, 3.0f)
  //     [SNew(SButton)
  //          .Text(ButtonCaption)
  //          .OnClicked(FOnClicked::CreateSP(
  //              this,
  //              &FCesiumGlobeAnchorCustomization::OnPlaceGeoreferenceOriginHere,
  //              TWeakObjectPtr<UFunction>(PlaceGeoreferenceOriginHere)))
  //          .ToolTipText(ButtonTooltip)];

  // CesiumCategory.AddCustomRow(ButtonSearchText.ToText())
  //     .RowTag(PlaceGeoreferenceOriginHere->GetFName())[WrapBox.ToSharedRef()];

  CesiumCategory.AddProperty(
      GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorComponent, Georeference));
  CesiumCategory.AddProperty(GET_MEMBER_NAME_CHECKED(
      UCesiumGlobeAnchorComponent,
      AdjustOrientationForGlobeWhenMoving));
  CesiumCategory.AddProperty(GET_MEMBER_NAME_CHECKED(
      UCesiumGlobeAnchorComponent,
      TeleportWhenUpdatingTransform));

  IDetailGroup& PositionLLH = CesiumCategory.AddGroup(
      "PositionLatitudeLongitudeHeight",
      FText::FromString("Position (Latitude, Longitude, Height)"),
      false,
      true);

  TSharedPtr<class IPropertyHandle> LatitudeDecimalDegreesHandle =
      DetailBuilder.GetProperty(
          GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorComponent, Latitude));
  IDetailPropertyRow& LatitudeRow =
      PositionLLH.AddPropertyRow(LatitudeDecimalDegreesHandle.ToSharedRef());
  LatitudeEditor = MakeShared<CesiumDegreesMinutesSecondsEditor>(
      LatitudeDecimalDegreesHandle,
      false);
  LatitudeEditor->PopulateRow(LatitudeRow);

  TSharedPtr<class IPropertyHandle> LongitudeDecimalDegreesHandle =
      DetailBuilder.GetProperty(
          GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorComponent, Longitude));
  IDetailPropertyRow& LongitudeRow =
      PositionLLH.AddPropertyRow(LongitudeDecimalDegreesHandle.ToSharedRef());
  LongitudeEditor = MakeShared<CesiumDegreesMinutesSecondsEditor>(
      LongitudeDecimalDegreesHandle,
      true);
  LongitudeEditor->PopulateRow(LongitudeRow);

  PositionLLH.AddPropertyRow(DetailBuilder.GetProperty(
      GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorComponent, Height)));

  IDetailGroup& PositionECEF = CesiumCategory.AddGroup(
      "PositionEarthCenteredEarthFixed",
      FText::FromString("Position (Earth-Centered, Earth-Fixed)"),
      false,
      true);

  PositionECEF
      .AddPropertyRow(DetailBuilder.GetProperty(
          GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorComponent, ECEF_X)))
      .DisplayName(FText::FromString("X"));
  PositionECEF
      .AddPropertyRow(DetailBuilder.GetProperty(
          GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorComponent, ECEF_Y)))
      .DisplayName(FText::FromString("Y"));
  PositionECEF
      .AddPropertyRow(DetailBuilder.GetProperty(
          GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorComponent, ECEF_Z)))
      .DisplayName(FText::FromString("Z"));

  this->CreateRotationEastSouthUp(DetailBuilder, CesiumCategory);
}

void FCesiumGlobeAnchorCustomization::CreateRotationEastSouthUp(
    IDetailLayoutBuilder& DetailBuilder,
    IDetailCategoryBuilder& Category) {
  IDetailGroup& RotationESU = Category.AddGroup(
      "RotationEastSouthUp",
      FText::FromString("Rotation (East-South-Up)"),
      false,
      true);

  this->UpdateEastSouthUpValues();

  TArrayView<UObject*> EastSouthUpPointerView = this->EastSouthUpPointers;
  TSharedPtr<IPropertyHandle> RollProperty =
      DetailBuilder.AddObjectPropertyData(EastSouthUpPointerView, "Roll");
  TSharedPtr<IPropertyHandle> PitchProperty =
      DetailBuilder.AddObjectPropertyData(EastSouthUpPointerView, "Pitch");
  TSharedPtr<IPropertyHandle> YawProperty =
      DetailBuilder.AddObjectPropertyData(EastSouthUpPointerView, "Yaw");

  RotationESU.AddPropertyRow(RollProperty.ToSharedRef());
  RotationESU.AddPropertyRow(PitchProperty.ToSharedRef());
  RotationESU.AddPropertyRow(YawProperty.ToSharedRef());

  TSharedPtr<IPropertyHandle> ActorToEcefProperty =
      DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(
          UCesiumGlobeAnchorComponent,
          ActorToEarthCenteredEarthFixedMatrix));
  ActorToEcefProperty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(
      this,
      &FCesiumGlobeAnchorCustomization::UpdateEastSouthUpValues));
}

void FCesiumGlobeAnchorCustomization::UpdateEastSouthUpValues() {
  this->EastSouthUpObjects.SetNum(this->SelectedObjects.Num());
  this->EastSouthUpPointers.SetNum(EastSouthUpObjects.Num());

  for (int i = 0; i < this->SelectedObjects.Num(); ++i) {
    if (!IsValid(this->EastSouthUpObjects[i].Get())) {
      this->EastSouthUpObjects[i] =
          NewObject<UCesiumGlobeAnchorRotationEastSouthUp>();
    }
    UCesiumGlobeAnchorComponent* GlobeAnchor =
        Cast<UCesiumGlobeAnchorComponent>(this->SelectedObjects[i]);
    if (!GlobeAnchor)
      continue;

    FQuat rotation = GlobeAnchor->GetEastSouthUpRotation();
    FRotator rotator = rotation.Rotator();
    this->EastSouthUpObjects[i]->Component = GlobeAnchor;
    this->EastSouthUpObjects[i]->Roll = rotator.Roll;
    this->EastSouthUpObjects[i]->Pitch = rotator.Pitch;
    this->EastSouthUpObjects[i]->Yaw = rotator.Yaw;
    this->EastSouthUpPointers[i] = this->EastSouthUpObjects[i].Get();
  }
}

FReply FCesiumGlobeAnchorCustomization::OnPlaceGeoreferenceOriginHere(
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
