// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "CesiumDmsEditor.h"
#include "CesiumRuntime.h"

#include "PropertyCustomizationHelpers.h"
#include "PropertyEditing.h"

#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Text/STextBlock.h"

#include <glm/glm.hpp>

/**
 * @brief A structure describing cartographic coordinates in
 * the DMS (Degree-Minute-Second) representation.
 */
struct DMS {

  /**
   * @brief The degrees.
   *
   * This is usually a value in [0,90] (for latitude) or
   * in [0,180] (for longitude), although explicit
   * clamping is not guaranteed.
   */
  int32_t d;

  /**
   * @brief The minutes.
   *
   * This is a value in [0,60).
   */
  int32_t m;

  /**
   * @brief The seconds.
   *
   * This is a value in [0,60).
   */
  double s;

  /**
   * @brief Whether the coordinate is negative.
   *
   * When the coordinate is negative, it represents a latitude south
   * of the equator, or a longitude west of the prime meridian.
   */
  bool negative;
};

/**
 * @brief Converts the given decimal degrees to a DMS representation.
 *
 * @param decimalDegrees The decimal degrees
 * @return The DMS representation.
 */
DMS decimalDegreesToDms(double decimalDegrees) {
  // Roughly based on
  // https://en.wikiversity.org/wiki/Geographic_coordinate_conversion
  // Section "Conversion_from_Decimal_Degree_to_DMS"
  bool negative = decimalDegrees < 0;
  double dd = negative ? -decimalDegrees : decimalDegrees;
  double d = glm::floor(dd);
  double min = (dd - d) * 60;
  double m = glm::floor(min);
  double sec = (min - m) * 60;
  double s = sec;
  if (s >= 60) {
    m++;
    s -= 60;
  }
  if (m == 60) {
    d++;
    m = 0;
  }
  return DMS{static_cast<int32_t>(d), static_cast<int32_t>(m), s, negative};
}

/**
 * @brief Converts the given DMS into decimal degrees.
 *
 * @param dms The DMS
 * @return The decimal degrees
 */
double dmsToDecimalDegrees(const DMS& dms) {
  double dd = dms.d + (dms.m / 60.0) + (dms.s / 3600.0);
  if (dms.negative) {
    return -dd;
  }
  return dd;
}

CesiumDmsEditor::CesiumDmsEditor(
    TSharedPtr<class IPropertyHandle> InputDecimalDegreesHandle,
    bool InputIsLongitude) {
  this->DecimalDegreesHandle = InputDecimalDegreesHandle;
  this->IsLongitude = InputIsLongitude;
}

void CesiumDmsEditor::PopulateRow(IDetailPropertyRow& Row) {

  // The default editing component for the property:
  // A SpinBox for the decimal degrees
  DecimalDegreesSpinBox =
      SNew(SSpinBox<double>)
          .MinSliderValue(IsLongitude ? -180 : -90)
          .MaxSliderValue(IsLongitude ? 180 : 90)
          .OnValueChanged(this, &CesiumDmsEditor::SetDecimalDegreesOnProperty)
          .Value(this, &CesiumDmsEditor::GetDecimalDegreesFromProperty);

  // Editing components for the DMS representation:
  // Spin boxes for degrees, minutes and seconds
  DegreesSpinBox = SNew(SSpinBox<int32>)
                       .MinSliderValue(0)
                       .MaxSliderValue(IsLongitude ? 179 : 89)
                       .OnValueChanged(this, &CesiumDmsEditor::SetDegrees)
                       .Value(this, &CesiumDmsEditor::GetDegrees);

  MinutesSpinBox = SNew(SSpinBox<int32>)
                       .MinSliderValue(0)
                       .MaxSliderValue(59)
                       .OnValueChanged(this, &CesiumDmsEditor::SetMinutes)
                       .Value(this, &CesiumDmsEditor::GetMinutes);

  SecondsSpinBox = SNew(SSpinBox<double>)
                       .MinSliderValue(0)
                       .MaxSliderValue(59.999999) // TODO This is ugly :-(
                       .OnValueChanged(this, &CesiumDmsEditor::SetSeconds)
                       .Value(this, &CesiumDmsEditor::GetSeconds);

  // The combo box for selecting "Eeast" or "West",
  // or "North" or "South", respectively.
  if (IsLongitude) {
    PositiveIndicator = MakeShareable(new FString(TEXT("E")));
    NegativeIndicator = MakeShareable(new FString(TEXT("W")));
  } else {
    PositiveIndicator = MakeShareable(new FString(TEXT("N")));
    NegativeIndicator = MakeShareable(new FString(TEXT("S")));
  }
  SignComboBoxItems.Add(NegativeIndicator);
  SignComboBoxItems.Emplace(PositiveIndicator);
  SignComboBox = SNew(STextComboBox)
                     .OptionsSource(&SignComboBoxItems)
                     .OnSelectionChanged(this, &CesiumDmsEditor::SignChanged);
  SignComboBox->SetSelectedItem(
      GetDecimalDegreesFromProperty() < 0 ? NegativeIndicator
                                          : PositiveIndicator);

  const float hPad = 3.0;
  const float vPad = 2.0;
  // clang-format off
	Row.CustomWidget().NameContent()
		[
			DecimalDegreesHandle->CreatePropertyNameWidget()
		]
		.ValueContent().HAlign(EHorizontalAlignment::HAlign_Fill)
		[
        SNew( SVerticalBox )
        + SVerticalBox::Slot().Padding(0.0f, vPad)
        [
              DecimalDegreesSpinBox.ToSharedRef()
        ]
        + SVerticalBox::Slot().Padding(0.0f, vPad)
        [
            SNew( SHorizontalBox )
            + SHorizontalBox::Slot().FillWidth(1.0)
            [
                DegreesSpinBox.ToSharedRef()
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(hPad, 0.0f)
            [
                SNew(STextBlock).Text(FText::FromString(TEXT("\u00B0")))
            ]
            + SHorizontalBox::Slot().FillWidth(1.0)
            [
                MinutesSpinBox.ToSharedRef()
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(hPad, 0.0f)
            [
                SNew(STextBlock).Text(FText::FromString(TEXT("\u2032")))
            ]
            + SHorizontalBox::Slot().FillWidth(1.0)
            [
                SecondsSpinBox.ToSharedRef()
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(hPad, 0.0f)
            [
                SNew(STextBlock).Text(FText::FromString(TEXT("\u2033")))
            ]
            + SHorizontalBox::Slot().AutoWidth()
            [
                SignComboBox.ToSharedRef()
            ]
        ]
		];
  // clang-format on
}

double CesiumDmsEditor::GetDecimalDegreesFromProperty() const {
  // UE_LOG(LogTemp, Warning, TEXT("GetDecimalDegreesFromProperty"));
  double decimalDegrees;
  FPropertyAccess::Result AccessResult =
      DecimalDegreesHandle->GetValue(decimalDegrees);
  if (AccessResult == FPropertyAccess::Success) {
    return decimalDegrees;
  }
  UE_LOG(LogCesium, Warning, TEXT("GetDecimalDegreesFromProperty FAILED"));
  return decimalDegrees;
}

void CesiumDmsEditor::SetDecimalDegreesOnProperty(double NewValue) {
  UE_LOG(LogTemp, Warning, TEXT("SetDecimalDegreesOnProperty %f"), NewValue);
  DecimalDegreesHandle->SetValue(NewValue);

  SignComboBox->SetSelectedItem(
      NewValue < 0 ? NegativeIndicator : PositiveIndicator);
}

int32 CesiumDmsEditor::GetDegrees() const {
  // UE_LOG(LogTemp, Warning, TEXT("GetDegrees"));
  double decimalDegrees = GetDecimalDegreesFromProperty();
  DMS dms = decimalDegreesToDms(decimalDegrees);
  return static_cast<int32>(dms.d);
}
void CesiumDmsEditor::SetDegrees(int32 NewValue) {
  UE_LOG(LogTemp, Warning, TEXT("SetDegrees %d"), NewValue);
  double decimalDegrees = GetDecimalDegreesFromProperty();
  DMS dms = decimalDegreesToDms(decimalDegrees);
  dms.d = NewValue;
  double newDecimalDegreesValue = dmsToDecimalDegrees(dms);
  SetDecimalDegreesOnProperty(newDecimalDegreesValue);
}

int32 CesiumDmsEditor::GetMinutes() const {
  // UE_LOG(LogTemp, Warning, TEXT("GetMinutes"));
  double decimalDegrees = GetDecimalDegreesFromProperty();
  DMS dms = decimalDegreesToDms(decimalDegrees);
  return static_cast<int32>(dms.m);
}
void CesiumDmsEditor::SetMinutes(int32 NewValue) {
  UE_LOG(LogTemp, Warning, TEXT("SetMinutes %d"), NewValue);
  double decimalDegrees = GetDecimalDegreesFromProperty();
  DMS dms = decimalDegreesToDms(decimalDegrees);
  dms.m = NewValue;
  double newDecimalDegreesValue = dmsToDecimalDegrees(dms);
  SetDecimalDegreesOnProperty(newDecimalDegreesValue);
}

double CesiumDmsEditor::GetSeconds() const {
  // UE_LOG(LogTemp, Warning, TEXT("GetSeconds"));
  double decimalDegrees = GetDecimalDegreesFromProperty();
  DMS dms = decimalDegreesToDms(decimalDegrees);
  return dms.s;
}
void CesiumDmsEditor::SetSeconds(double NewValue) {
  UE_LOG(LogTemp, Warning, TEXT("SetSeconds %f"), NewValue);
  double decimalDegrees = GetDecimalDegreesFromProperty();
  DMS dms = decimalDegreesToDms(decimalDegrees);
  dms.s = NewValue;
  double newDecimalDegreesValue = dmsToDecimalDegrees(dms);
  SetDecimalDegreesOnProperty(newDecimalDegreesValue);
}

void CesiumDmsEditor::SignChanged(
    TSharedPtr<FString> StringItem,
    ESelectInfo::Type SelectInfo) {

  bool negative = false;
  if (StringItem.IsValid()) {
    negative = (StringItem == NegativeIndicator);
  }
  UE_LOG(LogTemp, Warning, TEXT("SignChanged"));
  double decimalDegrees = GetDecimalDegreesFromProperty();
  DMS dms = decimalDegreesToDms(decimalDegrees);
  dms.negative = negative;
  double newDecimalDegreesValue = dmsToDecimalDegrees(dms);
  SetDecimalDegreesOnProperty(newDecimalDegreesValue);
}

#endif // WITH_EDITOR
