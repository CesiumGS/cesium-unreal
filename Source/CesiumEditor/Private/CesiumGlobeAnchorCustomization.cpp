// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumGlobeAnchorCustomization.h"
#include "CesiumCustomization.h"
#include "CesiumDegreesMinutesSecondsEditor.h"
#include "CesiumGlobeAnchorComponent.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "IDetailGroup.h"

void UCesiumGlobeAnchorRotationEastSouthUp::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) {
  Super::PostEditChangeProperty(PropertyChangedEvent);
  this->GlobeAnchor->Modify();
  this->GlobeAnchor->SetEastSouthUpRotation(
      FRotator(this->Pitch, this->Yaw, this->Roll).Quaternion());
}

void UCesiumGlobeAnchorRotationEastSouthUp::Initialize(
    UCesiumGlobeAnchorComponent* GlobeAnchorComponent) {
  this->GlobeAnchor = GlobeAnchorComponent;
  this->Tick(0.0f);
}

void UCesiumGlobeAnchorRotationEastSouthUp::Tick(float DeltaTime) {
  if (this->GlobeAnchor) {
    FQuat rotation = this->GlobeAnchor->GetEastSouthUpRotation();
    FRotator rotator = rotation.Rotator();
    this->Roll = rotator.Roll;
    this->Pitch = rotator.Pitch;
    this->Yaw = rotator.Yaw;
  }
}

TStatId UCesiumGlobeAnchorRotationEastSouthUp::GetStatId() const {
  RETURN_QUICK_DECLARE_CYCLE_STAT(
      UCesiumGlobeAnchorRotationEastSouthUp,
      STATGROUP_Tickables);
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

  TSharedPtr<CesiumButtonGroup> pButtons =
      CesiumCustomization::CreateButtonGroup();
  pButtons->AddButtonForUFunction(
      UCesiumGlobeAnchorComponent::StaticClass()->FindFunctionByName(
          GET_FUNCTION_NAME_CHECKED(
              UCesiumGlobeAnchorComponent,
              SnapLocalUpToEllipsoidNormal)));

  pButtons->AddButtonForUFunction(
      UCesiumGlobeAnchorComponent::StaticClass()->FindFunctionByName(
          GET_FUNCTION_NAME_CHECKED(
              UCesiumGlobeAnchorComponent,
              SnapToEastSouthUp)));

  pButtons->Finish(DetailBuilder, CesiumCategory);

  CesiumCategory.AddProperty(
      GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorComponent, Georeference));
  CesiumCategory.AddProperty(GET_MEMBER_NAME_CHECKED(
      UCesiumGlobeAnchorComponent,
      AdjustOrientationForGlobeWhenMoving));
  CesiumCategory.AddProperty(GET_MEMBER_NAME_CHECKED(
      UCesiumGlobeAnchorComponent,
      TeleportWhenUpdatingTransform));

  IDetailGroup& PositionLLH = CesiumCustomization::CreateGroup(
      CesiumCategory,
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

  IDetailGroup& PositionECEF = CesiumCustomization::CreateGroup(
      CesiumCategory,
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
  IDetailGroup& RotationESU = CesiumCustomization::CreateGroup(
      Category,
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

    this->EastSouthUpObjects[i]->Initialize(GlobeAnchor);
    this->EastSouthUpPointers[i] = this->EastSouthUpObjects[i].Get();
  }
}
