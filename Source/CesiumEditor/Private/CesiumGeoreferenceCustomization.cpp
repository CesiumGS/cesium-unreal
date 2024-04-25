// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumGeoreferenceCustomization.h"
#include "CesiumCustomization.h"
#include "CesiumDegreesMinutesSecondsEditor.h"
#include "CesiumGeoreference.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"

FName FCesiumGeoreferenceCustomization::RegisteredLayoutName;

void FCesiumGeoreferenceCustomization::Register(
    FPropertyEditorModule& PropertyEditorModule) {

  RegisteredLayoutName = ACesiumGeoreference::StaticClass()->GetFName();

  PropertyEditorModule.RegisterCustomClassLayout(
      RegisteredLayoutName,
      FOnGetDetailCustomizationInstance::CreateStatic(
          &FCesiumGeoreferenceCustomization::MakeInstance));
}

void FCesiumGeoreferenceCustomization::Unregister(
    FPropertyEditorModule& PropertyEditorModule) {
  PropertyEditorModule.UnregisterCustomClassLayout(RegisteredLayoutName);
}

TSharedRef<IDetailCustomization>
FCesiumGeoreferenceCustomization::MakeInstance() {
  return MakeShareable(new FCesiumGeoreferenceCustomization);
}

void FCesiumGeoreferenceCustomization::CustomizeDetails(
    IDetailLayoutBuilder& DetailBuilder) {
  IDetailCategoryBuilder& CesiumCategory = DetailBuilder.EditCategory("Cesium");

  TSharedPtr<CesiumButtonGroup> pButtons =
      CesiumCustomization::CreateButtonGroup();

  pButtons->AddButtonForUFunction(
      ACesiumGeoreference::StaticClass()->FindFunctionByName(
          GET_FUNCTION_NAME_CHECKED(
              ACesiumGeoreference,
              PlaceGeoreferenceOriginHere)));

  pButtons->AddButtonForUFunction(
      ACesiumGeoreference::StaticClass()->FindFunctionByName(
          GET_FUNCTION_NAME_CHECKED(ACesiumGeoreference, CreateSubLevelHere)));

  pButtons->Finish(DetailBuilder, CesiumCategory);

  CesiumCategory.AddProperty(
      GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, OriginPlacement));

  TSharedPtr<class IPropertyHandle> LatitudeDecimalDegreesHandle =
      DetailBuilder.GetProperty(
          GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, OriginLatitude));
  IDetailPropertyRow& LatitudeRow =
      CesiumCategory.AddProperty(LatitudeDecimalDegreesHandle);
  LatitudeEditor = MakeShared<CesiumDegreesMinutesSecondsEditor>(
      LatitudeDecimalDegreesHandle,
      false);
  LatitudeEditor->PopulateRow(LatitudeRow);

  TSharedPtr<class IPropertyHandle> LongitudeDecimalDegreesHandle =
      DetailBuilder.GetProperty(
          GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, OriginLongitude));
  IDetailPropertyRow& LongitudeRow =
      CesiumCategory.AddProperty(LongitudeDecimalDegreesHandle);
  LongitudeEditor = MakeShared<CesiumDegreesMinutesSecondsEditor>(
      LongitudeDecimalDegreesHandle,
      true);
  LongitudeEditor->PopulateRow(LongitudeRow);

  CesiumCategory.AddProperty(
      GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, OriginHeight));

  CesiumCategory.AddProperty(
      GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, Scale));
  CesiumCategory.AddProperty(
      GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, ShowLoadRadii));
}
