// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "CesiumGeoreferenceCustomization.h"
#include "CesiumDmsEditor.h"
#include "CesiumGeoreference.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditing.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<IDetailCustomization>
FCesiumGeoreferenceCustomization::MakeInstance() {
  return MakeShareable(new FCesiumGeoreferenceCustomization);
}

void FCesiumGeoreferenceCustomization::CustomizeDetails(
    IDetailLayoutBuilder& DetailBuilder) {
  IDetailCategoryBuilder& CesiumCategory = DetailBuilder.EditCategory("Cesium");

  CesiumCategory.AddProperty(GET_FUNCTION_NAME_CHECKED(
      ACesiumGeoreference,
      PlaceGeoreferenceOriginHere));
  CesiumCategory.AddProperty(
      GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, OriginPlacement));

  TSharedPtr<class IPropertyHandle> LongitudeDecimalDegreesHandle =
      DetailBuilder.GetProperty(
          GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, OriginLongitude));
  IDetailPropertyRow& LongitudeRow =
      CesiumCategory.AddProperty(LongitudeDecimalDegreesHandle);
  LongitudeEditor =
      MakeShared<CesiumDmsEditor>(LongitudeDecimalDegreesHandle, true);
  LongitudeEditor->PopulateRow(LongitudeRow);

  TSharedPtr<class IPropertyHandle> LatitudeDecimalDegreesHandle =
      DetailBuilder.GetProperty(
          GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, OriginLatitude));
  IDetailPropertyRow& LatitudeRow =
      CesiumCategory.AddProperty(LatitudeDecimalDegreesHandle);
  LatitudeEditor =
      MakeShared<CesiumDmsEditor>(LatitudeDecimalDegreesHandle, false);
  LatitudeEditor->PopulateRow(LatitudeRow);

  CesiumCategory.AddProperty(
      GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, OriginHeight));
  CesiumCategory.AddProperty(
      GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, KeepWorldOriginNearCamera));
  CesiumCategory.AddProperty(GET_MEMBER_NAME_CHECKED(
      ACesiumGeoreference,
      MaximumWorldOriginDistanceFromCamera));
  CesiumCategory.AddProperty(
      GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, WorldOriginCamera));
}

#endif // WITH_EDITOR
