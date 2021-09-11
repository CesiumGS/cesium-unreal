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
  // TODO Necessary?
  /*
  // skip customization if select more than one objects
  TArray<TWeakObjectPtr<UObject>> Objects;
  DetailBuilder.GetObjectsBeingCustomized(Objects);
    if (Objects.Num() != 1)
    {
        return;
    }
    */
  IDetailCategoryBuilder& CesiumCategory = DetailBuilder.EditCategory("Cesium");


  DecimalDegreesHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(ACesiumGeoreference, OriginLongitude));
	IDetailPropertyRow& Row = CesiumCategory.AddProperty(DecimalDegreesHandle);

  DecimalDegreesSpinBox = SNew( SSpinBox<double> )
              .MinSliderValue( -180 )
              .MaxSliderValue( 180 )
              .OnValueChanged(this, &FCesiumGeoreferenceCustomization::SetDecimalDegreesOnProperty )  
              .Value(this, &FCesiumGeoreferenceCustomization::GetDecimalDegreesFromProperty);

  DegreesSpinBox = SNew( SSpinBox<int32> )
              .MinSliderValue( 0 )
              .MaxSliderValue( 179 )
              .OnValueChanged(this, &FCesiumGeoreferenceCustomization::SetDegrees )  
              .Value(this, &FCesiumGeoreferenceCustomization::GetDegrees);

  MinutesSpinBox = SNew( SSpinBox<int32> )
              .MinSliderValue( 0 )
              .MaxSliderValue( 59 )
              .OnValueChanged(this, &FCesiumGeoreferenceCustomization::SetMinutes )  
              .Value(this, &FCesiumGeoreferenceCustomization::GetMinutes);

  SecondsSpinBox = SNew( SSpinBox<double> )
              .MinSliderValue( 0 )
              .MaxSliderValue( 59.999999 ) // TODO This is ugly :-(
              .OnValueChanged(this, &FCesiumGeoreferenceCustomization::SetSeconds )  
              .Value(this, &FCesiumGeoreferenceCustomization::GetSeconds);

  NegativeIndicator = MakeShareable(new FString(TEXT("W")));
  PositiveIndicator = MakeShareable(new FString(TEXT("E")));
  SignComboBoxItems.Add(NegativeIndicator);
  SignComboBoxItems.Emplace(PositiveIndicator);
  SignComboBox = SNew(STextComboBox)
			.OptionsSource(&SignComboBoxItems)
			.OnSelectionChanged(this, &FCesiumGeoreferenceCustomization::SignChanged);
  SignComboBox->SetSelectedItem(
    GetDecimalDegreesFromProperty() < 0 ? NegativeIndicator : PositiveIndicator);

  const float hPad = 2.0;
  // clang-format off
	Row.CustomWidget()
		.NameContent()
		[
			DecimalDegreesHandle->CreatePropertyNameWidget()
		]
		.ValueContent().HAlign(EHorizontalAlignment::HAlign_Fill)
		[
        SNew( SVerticalBox )
        + SVerticalBox::Slot()
        [
              DecimalDegreesSpinBox.ToSharedRef()
        ]
        + SVerticalBox::Slot()
        [
            SNew( SHorizontalBox )
            + SHorizontalBox::Slot().FillWidth(1.0)
            [
                DegreesSpinBox.ToSharedRef()
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(hPad, 0.0f)
            [
                SNew(STextBlock)
                  .Text(FText::FromString(TEXT("°")))
            ]
            + SHorizontalBox::Slot().FillWidth(1.0)
            [
                MinutesSpinBox.ToSharedRef()
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(hPad, 0.0f)
            [
                SNew(STextBlock)
                  .Text(FText::FromString(TEXT("'")))
            ]
            + SHorizontalBox::Slot().FillWidth(1.0)
            [
                SecondsSpinBox.ToSharedRef()
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


  double FCesiumGeoreferenceCustomization::GetDecimalDegreesFromProperty() const
  {
    UE_LOG(LogTemp, Warning, TEXT("GetDecimalDegreesFromProperty"));
    // TODO: HANDLE FAILURE CASES
    double Value;
    FPropertyAccess::Result AccessResult = DecimalDegreesHandle->GetValue( Value );
    if (AccessResult == FPropertyAccess::Success) {
      return Value;
    }
    UE_LOG(LogTemp, Warning, TEXT("GetDecimalDegreesFromProperty FAILED"));
    return Value;
  }

  void FCesiumGeoreferenceCustomization::SetDecimalDegreesOnProperty( double NewValue)
  {
    UE_LOG(LogTemp, Warning, TEXT("SetDecimalDegreesOnProperty"));
    DecimalDegreesHandle->SetValue( NewValue );

    SignComboBox->SetSelectedItem(
      NewValue < 0 ? NegativeIndicator : PositiveIndicator);
  }


  int32 FCesiumGeoreferenceCustomization::GetDegrees() const {
    UE_LOG(LogTemp, Warning, TEXT("GetDegrees"));
    double decimalDegrees = GetDecimalDegreesFromProperty();
    DMS dms = decimalDegreesToDms(decimalDegrees);
    return static_cast<int32>(dms.d);
  }
  void FCesiumGeoreferenceCustomization::SetDegrees( int32 NewValue) {
    UE_LOG(LogTemp, Warning, TEXT("SetDegrees"));
    double decimalDegrees = GetDecimalDegreesFromProperty();
    DMS dms = decimalDegreesToDms(decimalDegrees);
    dms.d = NewValue;
    double newDecimalDegreesValue = dmsToDecimalDegrees(dms);
    SetDecimalDegreesOnProperty(newDecimalDegreesValue);
  }

  int32 FCesiumGeoreferenceCustomization::GetMinutes() const {
    UE_LOG(LogTemp, Warning, TEXT("GetMinutes"));
    double decimalDegrees = GetDecimalDegreesFromProperty();
    DMS dms = decimalDegreesToDms(decimalDegrees);
    return static_cast<int32>(dms.m);
  }
  void FCesiumGeoreferenceCustomization::SetMinutes( int32 NewValue) {
    UE_LOG(LogTemp, Warning, TEXT("SetMinutes"));
    double decimalDegrees = GetDecimalDegreesFromProperty();
    DMS dms = decimalDegreesToDms(decimalDegrees);
    dms.m = NewValue;
    double newDecimalDegreesValue = dmsToDecimalDegrees(dms);
    SetDecimalDegreesOnProperty(newDecimalDegreesValue);
  }

  double FCesiumGeoreferenceCustomization::GetSeconds() const {
    UE_LOG(LogTemp, Warning, TEXT("GetSeconds"));
    double decimalDegrees = GetDecimalDegreesFromProperty();
    DMS dms = decimalDegreesToDms(decimalDegrees);
    return dms.s;
  }
  void FCesiumGeoreferenceCustomization::SetSeconds( double NewValue) {
    UE_LOG(LogTemp, Warning, TEXT("SetSeconds"));
    double decimalDegrees = GetDecimalDegreesFromProperty();
    DMS dms = decimalDegreesToDms(decimalDegrees);
    dms.s = NewValue;
    double newDecimalDegreesValue = dmsToDecimalDegrees(dms);
    SetDecimalDegreesOnProperty(newDecimalDegreesValue);
  }

  void FCesiumGeoreferenceCustomization::SignChanged(TSharedPtr<FString> StringItem, ESelectInfo::Type SelectInfo) {

    bool negative = false;
    if (StringItem.IsValid())
    {
      negative = (StringItem == NegativeIndicator);
    }
    UE_LOG(LogTemp, Warning, TEXT("SignChanged"));
    double decimalDegrees = GetDecimalDegreesFromProperty();
    DMS dms = decimalDegreesToDms(decimalDegrees);
    dms.negative = negative;
    double newDecimalDegreesValue = dmsToDecimalDegrees(dms);
    SetDecimalDegreesOnProperty(newDecimalDegreesValue);
  }


