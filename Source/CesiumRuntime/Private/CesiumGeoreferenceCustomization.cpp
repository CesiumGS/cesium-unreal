// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGeoreferenceCustomization.h"
#include "CesiumGeoreference.h"
#include "PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/STextComboBox.h"

#include <glm/glm.hpp>


struct DMS{
  double d;
  double m;
  double s;
  bool negative;
};

// Roughly based on https://en.wikiversity.org/wiki/Geographic_coordinate_conversion
// Section "Conversion_from_Decimal_Degree_to_DMS"
DMS decimalDegreesToDms(double decimalDegrees) {
  bool negative = decimalDegrees < 0;
  double dd = negative ? -decimalDegrees : decimalDegrees;
  double d = glm::floor(dd);
  double min = (dd-d)*60;
  double m = glm::floor(min);
  double sec = (min-m)*60;
  double s = sec;
  if (s>=60) {
    m++;
   s-=60;
  }
  if (m==60) {
    d++;
    m=0;
   }
  return DMS { d, m, s, negative };
}

double dmsToDecimalDegrees(const DMS& dms) {
  double dd = dms.d + (dms.m / 60) + (dms.s / 3600);
  if (dms.negative)
  {
    return -dd;
  }
  return dd;
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
              .MinSliderValue( 0 )
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
              .MaxSliderValue( 59.999999 ) // TODO This is ugly :-(
              .OnValueChanged(this, &FCesiumGeoreferenceCustomization::SetOriginLongitudeSeconds )  
              .Value(this, &FCesiumGeoreferenceCustomization::GetOriginLongitudeSeconds);

  NegativeIndicator = MakeShareable(new FString(TEXT("S")));
  PositiveIndicator = MakeShareable(new FString(TEXT("N")));
  SignComboBoxItems.Add(NegativeIndicator);
  SignComboBoxItems.Emplace(PositiveIndicator);
  SignComboBox = SNew(STextComboBox)
			.OptionsSource(&SignComboBoxItems)
			.OnSelectionChanged(this, &FCesiumGeoreferenceCustomization::SignChanged);
  SignComboBox->SetSelectedItem(
    GetOriginLongitudeFromProperty() < 0 ? NegativeIndicator : PositiveIndicator);

  const float hPad = 2.0;
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
            + SHorizontalBox::Slot().AutoWidth().Padding(hPad, 0.0f)
            [
                SNew(STextBlock)
                  .Text(FText::FromString(TEXT("°")))
            ]
            + SHorizontalBox::Slot().FillWidth(1.0)
            [
                OriginLongitudeMinutesSpinBox.ToSharedRef()
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(hPad, 0.0f)
            [
                SNew(STextBlock)
                  .Text(FText::FromString(TEXT("'")))
            ]
            + SHorizontalBox::Slot().FillWidth(1.0)
            [
                OriginLongitudeSecondsSpinBox.ToSharedRef()
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(hPad, 0.0f)
            [
                SNew(STextBlock)
                  .Text(FText::FromString(TEXT("\"")))
            ]
            + SHorizontalBox::Slot().FillWidth(0.5)
            [
                SignComboBox.ToSharedRef()
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

    SignComboBox->SetSelectedItem(
      NewValue < 0 ? NegativeIndicator : PositiveIndicator);
  }


  int32 FCesiumGeoreferenceCustomization::GetOriginLongitudeDegrees() const {
    UE_LOG(LogTemp, Warning, TEXT("GetOriginLongitudeDegrees"));
    double value = GetOriginLongitudeFromProperty();
    DMS dms = decimalDegreesToDms(value);
    return static_cast<int32>(dms.d);
  }
  void FCesiumGeoreferenceCustomization::SetOriginLongitudeDegrees( int32 NewValue) {
    UE_LOG(LogTemp, Warning, TEXT("SetOriginLongitudeDegrees"));
    double value = GetOriginLongitudeFromProperty();
    DMS dms = decimalDegreesToDms(value);
    dms.d = NewValue;
    double newDecimalDegreesValue = dmsToDecimalDegrees(dms);
    SetOriginLongitudeOnProperty(newDecimalDegreesValue);
  }

  int32 FCesiumGeoreferenceCustomization::GetOriginLongitudeMinutes() const {
    UE_LOG(LogTemp, Warning, TEXT("GetOriginLongitudeMinutes"));
    double value = GetOriginLongitudeFromProperty();
    DMS dms = decimalDegreesToDms(value);
    return static_cast<int32>(dms.m);
  }
  void FCesiumGeoreferenceCustomization::SetOriginLongitudeMinutes( int32 NewValue) {
    UE_LOG(LogTemp, Warning, TEXT("SetOriginLongitudeMinutes"));
    double value = GetOriginLongitudeFromProperty();
    DMS dms = decimalDegreesToDms(value);
    dms.m = NewValue;
    double newDecimalDegreesValue = dmsToDecimalDegrees(dms);
    SetOriginLongitudeOnProperty(newDecimalDegreesValue);
  }

  double FCesiumGeoreferenceCustomization::GetOriginLongitudeSeconds() const {
    UE_LOG(LogTemp, Warning, TEXT("GetOriginLongitudeSeconds"));
    double value = GetOriginLongitudeFromProperty();
    DMS dms = decimalDegreesToDms(value);
    return dms.s;
  }
  void FCesiumGeoreferenceCustomization::SetOriginLongitudeSeconds( double NewValue) {
    UE_LOG(LogTemp, Warning, TEXT("SetOriginLongitudeSeconds"));
    double value = GetOriginLongitudeFromProperty();
    DMS dms = decimalDegreesToDms(value);
    dms.s = NewValue;
    double newDecimalDegreesValue = dmsToDecimalDegrees(dms);
    SetOriginLongitudeOnProperty(newDecimalDegreesValue);
  }

  void FCesiumGeoreferenceCustomization::SignChanged(TSharedPtr<FString> StringItem, ESelectInfo::Type SelectInfo) {

    bool negative = false;
    if (StringItem.IsValid())
    {
      negative = (StringItem == NegativeIndicator);
    }
    UE_LOG(LogTemp, Warning, TEXT("SignChanged"));
    double value = GetOriginLongitudeFromProperty();
    DMS dms = decimalDegreesToDms(value);
    dms.negative = negative;
    double newDecimalDegreesValue = dmsToDecimalDegrees(dms);
    SetOriginLongitudeOnProperty(newDecimalDegreesValue);
  }


