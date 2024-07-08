// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGlobeAnchorCustomization.h"
#include "CesiumCustomization.h"
#include "CesiumDegreesMinutesSecondsEditor.h"
#include "CesiumGlobeAnchorComponent.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "IDetailGroup.h"
#include "Widgets/SToolTip.h"

FName FCesiumGlobeAnchorCustomization::RegisteredLayoutName;

void FCesiumGlobeAnchorCustomization::Register(
    FPropertyEditorModule& PropertyEditorModule) {

  RegisteredLayoutName = UCesiumGlobeAnchorComponent::StaticClass()->GetFName();

  PropertyEditorModule.RegisterCustomClassLayout(
      RegisteredLayoutName,
      FOnGetDetailCustomizationInstance::CreateStatic(
          &FCesiumGlobeAnchorCustomization::MakeInstance));
}

void FCesiumGlobeAnchorCustomization::Unregister(
    FPropertyEditorModule& PropertyEditorModule) {
  PropertyEditorModule.UnregisterCustomClassLayout(RegisteredLayoutName);
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
      ResolvedGeoreference));
  CesiumCategory.AddProperty(GET_MEMBER_NAME_CHECKED(
      UCesiumGlobeAnchorComponent,
      AdjustOrientationForGlobeWhenMoving));
  CesiumCategory.AddProperty(GET_MEMBER_NAME_CHECKED(
      UCesiumGlobeAnchorComponent,
      TeleportWhenUpdatingTransform));

  this->UpdateDerivedProperties();

  this->CreatePositionLongitudeLatitudeHeight(DetailBuilder, CesiumCategory);
  this->CreatePositionEarthCenteredEarthFixed(DetailBuilder, CesiumCategory);
  this->CreateRotationEastSouthUp(DetailBuilder, CesiumCategory);
}

void FCesiumGlobeAnchorCustomization::CreatePositionEarthCenteredEarthFixed(
    IDetailLayoutBuilder& DetailBuilder,
    IDetailCategoryBuilder& Category) {
  IDetailGroup& Group = CesiumCustomization::CreateGroup(
      Category,
      "PositionEarthCenteredEarthFixed",
      FText::FromString("Position (Earth-Centered, Earth-Fixed)"),
      false,
      true);

  TArrayView<UObject*> View = this->DerivedPointers;
  TSharedPtr<IPropertyHandle> XProperty = DetailBuilder.AddObjectPropertyData(
      View,
      GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorDerivedProperties, X));
  TSharedPtr<IPropertyHandle> YProperty = DetailBuilder.AddObjectPropertyData(
      View,
      GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorDerivedProperties, Y));
  TSharedPtr<IPropertyHandle> ZProperty = DetailBuilder.AddObjectPropertyData(
      View,
      GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorDerivedProperties, Z));

  Group.AddPropertyRow(XProperty.ToSharedRef());
  Group.AddPropertyRow(YProperty.ToSharedRef());
  Group.AddPropertyRow(ZProperty.ToSharedRef());
}

void FCesiumGlobeAnchorCustomization::CreatePositionLongitudeLatitudeHeight(
    IDetailLayoutBuilder& DetailBuilder,
    IDetailCategoryBuilder& Category) {
  IDetailGroup& Group = CesiumCustomization::CreateGroup(
      Category,
      "PositionLatitudeLongitudeHeight",
      FText::FromString("Position (Latitude, Longitude, Height)"),
      false,
      true);

  TArrayView<UObject*> View = this->DerivedPointers;
  TSharedPtr<IPropertyHandle> LatitudeProperty =
      DetailBuilder.AddObjectPropertyData(
          View,
          GET_MEMBER_NAME_CHECKED(
              UCesiumGlobeAnchorDerivedProperties,
              Latitude));
  TSharedPtr<IPropertyHandle> LongitudeProperty =
      DetailBuilder.AddObjectPropertyData(
          View,
          GET_MEMBER_NAME_CHECKED(
              UCesiumGlobeAnchorDerivedProperties,
              Longitude));
  TSharedPtr<IPropertyHandle> HeightProperty =
      DetailBuilder.AddObjectPropertyData(
          View,
          GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorDerivedProperties, Height));

  IDetailPropertyRow& LatitudeRow =
      Group.AddPropertyRow(LatitudeProperty.ToSharedRef());
  LatitudeEditor =
      MakeShared<CesiumDegreesMinutesSecondsEditor>(LatitudeProperty, false);
  LatitudeEditor->PopulateRow(LatitudeRow);

  IDetailPropertyRow& LongitudeRow =
      Group.AddPropertyRow(LongitudeProperty.ToSharedRef());
  LongitudeEditor =
      MakeShared<CesiumDegreesMinutesSecondsEditor>(LongitudeProperty, true);
  LongitudeEditor->PopulateRow(LongitudeRow);

  Group.AddPropertyRow(HeightProperty.ToSharedRef());
}

void FCesiumGlobeAnchorCustomization::CreateRotationEastSouthUp(
    IDetailLayoutBuilder& DetailBuilder,
    IDetailCategoryBuilder& Category) {
  IDetailGroup& Group = CesiumCustomization::CreateGroup(
      Category,
      "RotationEastSouthUp",
      FText::FromString("Rotation (East-South-Up)"),
      false,
      true);

  this->UpdateDerivedProperties();

  TArrayView<UObject*> EastSouthUpPointerView = this->DerivedPointers;
  TSharedPtr<IPropertyHandle> RollProperty =
      DetailBuilder.AddObjectPropertyData(EastSouthUpPointerView, "Roll");
  TSharedPtr<IPropertyHandle> PitchProperty =
      DetailBuilder.AddObjectPropertyData(EastSouthUpPointerView, "Pitch");
  TSharedPtr<IPropertyHandle> YawProperty =
      DetailBuilder.AddObjectPropertyData(EastSouthUpPointerView, "Yaw");

  Group.AddPropertyRow(RollProperty.ToSharedRef());
  Group.AddPropertyRow(PitchProperty.ToSharedRef());
  Group.AddPropertyRow(YawProperty.ToSharedRef());
}

void FCesiumGlobeAnchorCustomization::UpdateDerivedProperties() {
  this->DerivedObjects.SetNum(this->SelectedObjects.Num());
  this->DerivedPointers.SetNum(DerivedObjects.Num());

  for (int i = 0; i < this->SelectedObjects.Num(); ++i) {
    if (!IsValid(this->DerivedObjects[i].Get())) {
      this->DerivedObjects[i] =
          NewObject<UCesiumGlobeAnchorDerivedProperties>();
    }
    UCesiumGlobeAnchorComponent* GlobeAnchor =
        Cast<UCesiumGlobeAnchorComponent>(this->SelectedObjects[i]);
    this->DerivedObjects[i]->Initialize(GlobeAnchor);
    this->DerivedPointers[i] = this->DerivedObjects[i].Get();
  }
}

void UCesiumGlobeAnchorDerivedProperties::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) {
  Super::PostEditChangeProperty(PropertyChangedEvent);

  if (!PropertyChangedEvent.Property) {
    return;
  }

  FName propertyName = PropertyChangedEvent.Property->GetFName();

  if (propertyName ==
          GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorDerivedProperties, X) ||
      propertyName ==
          GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorDerivedProperties, Y) ||
      propertyName ==
          GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorDerivedProperties, Z)) {
    this->GlobeAnchor->Modify();
    this->GlobeAnchor->MoveToEarthCenteredEarthFixedPosition(
        FVector(this->X, this->Y, this->Z));
  } else if (true) {
    if (propertyName == GET_MEMBER_NAME_CHECKED(
                            UCesiumGlobeAnchorDerivedProperties,
                            Longitude) ||
        propertyName == GET_MEMBER_NAME_CHECKED(
                            UCesiumGlobeAnchorDerivedProperties,
                            Latitude) ||
        propertyName == GET_MEMBER_NAME_CHECKED(
                            UCesiumGlobeAnchorDerivedProperties,
                            Height)) {
      this->GlobeAnchor->Modify();
      this->GlobeAnchor->MoveToLongitudeLatitudeHeight(
          FVector(this->Longitude, this->Latitude, this->Height));
    } else if (
        propertyName == GET_MEMBER_NAME_CHECKED(
                            UCesiumGlobeAnchorDerivedProperties,
                            Pitch) ||
        propertyName ==
            GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorDerivedProperties, Yaw) ||
        propertyName == GET_MEMBER_NAME_CHECKED(
                            UCesiumGlobeAnchorDerivedProperties,
                            Roll)) {
      this->GlobeAnchor->Modify();
      this->GlobeAnchor->SetEastSouthUpRotation(
          FRotator(this->Pitch, this->Yaw, this->Roll).Quaternion());
    }
  }
}

bool UCesiumGlobeAnchorDerivedProperties::CanEditChange(
    const FProperty* InProperty) const {
  const FName Name = InProperty->GetFName();

  // Valid georeference, nothing to disable
  if (IsValid(this->GlobeAnchor->ResolveGeoreference())) {
    return true;
  }

  return Name != GET_MEMBER_NAME_CHECKED(
                     UCesiumGlobeAnchorDerivedProperties,
                     Longitude) &&
         Name != GET_MEMBER_NAME_CHECKED(
                     UCesiumGlobeAnchorDerivedProperties,
                     Latitude) &&
         Name != GET_MEMBER_NAME_CHECKED(
                     UCesiumGlobeAnchorDerivedProperties,
                     Height) &&
         Name != GET_MEMBER_NAME_CHECKED(
                     UCesiumGlobeAnchorDerivedProperties,
                     Pitch) &&
         Name != GET_MEMBER_NAME_CHECKED(
                     UCesiumGlobeAnchorDerivedProperties,
                     Yaw) &&
         Name !=
             GET_MEMBER_NAME_CHECKED(UCesiumGlobeAnchorDerivedProperties, Roll);
}

void UCesiumGlobeAnchorDerivedProperties::Initialize(
    UCesiumGlobeAnchorComponent* GlobeAnchorComponent) {
  this->GlobeAnchor = GlobeAnchorComponent;
  this->Tick(0.0f);
}

void UCesiumGlobeAnchorDerivedProperties::Tick(float DeltaTime) {
  if (this->GlobeAnchor) {
    FVector position = this->GlobeAnchor->GetEarthCenteredEarthFixedPosition();
    this->X = position.X;
    this->Y = position.Y;
    this->Z = position.Z;

    // We can't transform the GlobeAnchor's ECEF coordinates back to
    // cartographic & rotation without a valid georeference to know what
    // ellipsoid to use.
    if (IsValid(this->GlobeAnchor->ResolveGeoreference())) {
      FVector llh = this->GlobeAnchor->GetLongitudeLatitudeHeight();
      this->Longitude = llh.X;
      this->Latitude = llh.Y;
      this->Height = llh.Z;

      FQuat rotation = this->GlobeAnchor->GetEastSouthUpRotation();
      FRotator rotator = rotation.Rotator();
      this->Roll = rotator.Roll;
      this->Pitch = rotator.Pitch;
      this->Yaw = rotator.Yaw;
    }
  }
}

TStatId UCesiumGlobeAnchorDerivedProperties::GetStatId() const {
  RETURN_QUICK_DECLARE_CYCLE_STAT(
      UCesiumGlobeAnchorRotationEastSouthUp,
      STATGROUP_Tickables);
}
