// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGeoreferenceCustomization.h"
#include "CesiumGeoreference.h"
#include "PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SSpinBox.h"

#include <glm/glm.hpp>

struct DMS{
  double d;
  double m;
  double s;
};

// As in https://en.wikiversity.org/wiki/Geographic_coordinate_conversion#Conversion_from_Decimal_Degree_to_DMS
DMS degreesToDms(double degrees) {
  double d = glm::floor(degrees);
  double min = (degrees-d)*60;
  double m = glm::floor(min);
  double sec = (min-m)*60;
  double s = glm::round(sec);
  if (s==60) {
    m++;
   s=0;
  }
  if (m==60) {
    d++;
    m=0;
   }
  return DMS { d, m, s };
}

double dmsToDegrees(const DMS& dms) {
  return dms.d + (dms.m / 60) + (dms.s / 3600);
}


TSharedRef< IDetailCustomization > FCesiumGeoreferenceCustomization::MakeInstance()
{
    return MakeShareable(new FCesiumGeoreferenceCustomization);
}

void FCesiumGeoreferenceCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
  OriginLongitudeHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, OriginLongitude));

  IDetailCategoryBuilder& CesiumCategory = DetailBuilder.EditCategory("Cesium");

	IDetailPropertyRow& OriginLongitudeRow = CesiumCategory.AddProperty(OriginLongitudeHandle);

  OriginLongitudeSpinBox = SNew( SSpinBox<double> )
              .MinSliderValue( -180 )
              .MaxSliderValue( 180 )
              .OnValueChanged(this, &FCesiumGeoreferenceCustomization::SetOriginLongitudeOnProperty )  
              .Value(this, &FCesiumGeoreferenceCustomization::GetOriginLongitudeFromProperty);

  OriginLongitudeDegreesSpinBox = SNew( SSpinBox<int32> )
              .MinSliderValue( -179 )
              .MaxSliderValue( 179 )
              .OnValueChanged(this, &FCesiumGeoreferenceCustomization::SetOriginLongitudeDegrees )  
              .Value(this, &FCesiumGeoreferenceCustomization::GetOriginLongitudeDegrees);

  OriginLongitudeMinutesSpinBox = SNew( SSpinBox<int32> )
              .MinSliderValue( 0 )
              .MaxSliderValue( 59 )
              .OnValueChanged(this, &FCesiumGeoreferenceCustomization::SetOriginLongitudeMinutes )  
              .Value(this, &FCesiumGeoreferenceCustomization::GetOriginLongitudeMinutes);

  OriginLongitudeSecondsSpinBox = SNew( SSpinBox<double> )
              .MinSliderValue( 0 )
              .MaxSliderValue( 60 )
              .OnValueChanged(this, &FCesiumGeoreferenceCustomization::SetOriginLongitudeSeconds )  
              .Value(this, &FCesiumGeoreferenceCustomization::GetOriginLongitudeSeconds);

  // clang-format off
	OriginLongitudeRow.CustomWidget()
		.NameContent()
		[
			OriginLongitudeHandle->CreatePropertyNameWidget()
		]
		.ValueContent().HAlign(EHorizontalAlignment::HAlign_Fill)
		[
        SNew( SVerticalBox )
        + SVerticalBox::Slot()
        [
              OriginLongitudeSpinBox.ToSharedRef()
        ]
        + SVerticalBox::Slot()
        [
            SNew( SHorizontalBox )
            + SHorizontalBox::Slot().FillWidth(1.0)
            [
                OriginLongitudeDegreesSpinBox.ToSharedRef()
            ]
            + SHorizontalBox::Slot().FillWidth(1.0)
            [
                OriginLongitudeMinutesSpinBox.ToSharedRef()
            ]
            + SHorizontalBox::Slot().FillWidth(1.0)
            [
                OriginLongitudeSecondsSpinBox.ToSharedRef()
            ]
        ]
		];
  // clang-format on
}


  double FCesiumGeoreferenceCustomization::GetOriginLongitudeFromProperty() const
  {
    UE_LOG(LogTemp, Warning, TEXT("GetOriginLongitudeFromProperty"));
    // TODO: HANDLE FAILURE CASES
    double Value;
    FPropertyAccess::Result AccessResult = OriginLongitudeHandle->GetValue( Value );
    if (AccessResult == FPropertyAccess::Success) {
      return Value;
    }
    UE_LOG(LogTemp, Warning, TEXT("GetOriginLongitudeFromProperty FAILED"));
    return Value;
  }

  void FCesiumGeoreferenceCustomization::SetOriginLongitudeOnProperty( double NewValue)
  {
    UE_LOG(LogTemp, Warning, TEXT("SetOriginLongitudeOnProperty"));
        OriginLongitudeHandle->SetValue( NewValue );
  }


  int32 FCesiumGeoreferenceCustomization::GetOriginLongitudeDegrees() const {
    UE_LOG(LogTemp, Warning, TEXT("GetOriginLongitudeDegrees"));
    double value = GetOriginLongitudeFromProperty();
    DMS dms = degreesToDms(value);
    return static_cast<int32>(dms.d);
  }
  void FCesiumGeoreferenceCustomization::SetOriginLongitudeDegrees( int32 NewValue) {
    UE_LOG(LogTemp, Warning, TEXT("SetOriginLongitudeDegrees"));
    double value = GetOriginLongitudeFromProperty();
    DMS dms = degreesToDms(value);
    dms.d = NewValue;
    double newPropertyValue = dmsToDegrees(dms);
    SetOriginLongitudeOnProperty(newPropertyValue);
  }

  int32 FCesiumGeoreferenceCustomization::GetOriginLongitudeMinutes() const {
    UE_LOG(LogTemp, Warning, TEXT("GetOriginLongitudeMinutes"));
    double value = GetOriginLongitudeFromProperty();
    DMS dms = degreesToDms(value);
    return static_cast<int32>(dms.m);
  }
  void FCesiumGeoreferenceCustomization::SetOriginLongitudeMinutes( int32 NewValue) {
    UE_LOG(LogTemp, Warning, TEXT("SetOriginLongitudeMinutes"));
    double value = GetOriginLongitudeFromProperty();
    DMS dms = degreesToDms(value);
    dms.m = NewValue;
    double newPropertyValue = dmsToDegrees(dms);
    SetOriginLongitudeOnProperty(newPropertyValue);
  }

  double FCesiumGeoreferenceCustomization::GetOriginLongitudeSeconds() const {
    UE_LOG(LogTemp, Warning, TEXT("GetOriginLongitudeSeconds"));
    double value = GetOriginLongitudeFromProperty();
    DMS dms = degreesToDms(value);
    return dms.s;
  }
  void FCesiumGeoreferenceCustomization::SetOriginLongitudeSeconds( double NewValue) {
    UE_LOG(LogTemp, Warning, TEXT("SetOriginLongitudeSeconds"));
    double value = GetOriginLongitudeFromProperty();
    DMS dms = degreesToDms(value);
    dms.s = NewValue;
    double newPropertyValue = dmsToDegrees(dms);
    SetOriginLongitudeOnProperty(newPropertyValue);
  }



