// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumDegreesMinutesSecondsEditor.h"
#include "CesiumEditor.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Text/STextBlock.h"

#include <glm/glm.hpp>

namespace {

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

} // namespace

CesiumDegreesMinutesSecondsEditor::CesiumDegreesMinutesSecondsEditor(
    TSharedPtr<class IPropertyHandle> InputDecimalDegreesHandle,
    bool InputIsLongitude) {
  this->DecimalDegreesHandle = InputDecimalDegreesHandle;
  this->IsLongitude = InputIsLongitude;
}

void CesiumDegreesMinutesSecondsEditor::PopulateRow(IDetailPropertyRow& Row) {
  FSlateFontInfo FontInfo = IDetailLayoutBuilder::GetDetailFont();

  // The default editing component for the property:
  // A SpinBox for the decimal degrees
  DecimalDegreesSpinBox =
      SNew(SSpinBox<double>)
          .Font(FontInfo)
          .MinSliderValue(IsLongitude ? -180 : -90)
          .MaxSliderValue(IsLongitude ? 180 : 90)
          .OnValueChanged(
              this,
              &CesiumDegreesMinutesSecondsEditor::SetDecimalDegreesOnProperty)
          .Value(
              this,
              &CesiumDegreesMinutesSecondsEditor::
                  GetDecimalDegreesFromProperty);

  // Editing components for the DMS representation:
  // Spin boxes for degrees, minutes and seconds
  DegreesSpinBox =
      SNew(SSpinBox<int32>)
          .Font(FontInfo)
          .ToolTipText(FText::FromString("Degrees"))
          .MinSliderValue(0)
          .MaxSliderValue(IsLongitude ? 179 : 89)
          .OnValueChanged(this, &CesiumDegreesMinutesSecondsEditor::SetDegrees)
          .Value(this, &CesiumDegreesMinutesSecondsEditor::GetDegrees);

  MinutesSpinBox =
      SNew(SSpinBox<int32>)
          .Font(FontInfo)
          .ToolTipText(FText::FromString("Minutes"))
          .MinSliderValue(0)
          .MaxSliderValue(59)
          .OnValueChanged(this, &CesiumDegreesMinutesSecondsEditor::SetMinutes)
          .Value(this, &CesiumDegreesMinutesSecondsEditor::GetMinutes);

  SecondsSpinBox =
      SNew(SSpinBox<double>)
          .Font(FontInfo)
          .ToolTipText(FText::FromString("Seconds"))
          .MinSliderValue(0)
          .MaxSliderValue(59.999999)
          .OnValueChanged(this, &CesiumDegreesMinutesSecondsEditor::SetSeconds)
          .Value(this, &CesiumDegreesMinutesSecondsEditor::GetSeconds);

  // The combo box for selecting "Eeast" or "West",
  // or "North" or "South", respectively.
  FText signTooltip;
  if (IsLongitude) {
    PositiveIndicator = MakeShareable(new FString(TEXT("E")));
    NegativeIndicator = MakeShareable(new FString(TEXT("W")));
    signTooltip = FText::FromString("East or West");
  } else {
    PositiveIndicator = MakeShareable(new FString(TEXT("N")));
    NegativeIndicator = MakeShareable(new FString(TEXT("S")));
    signTooltip = FText::FromString("North or South");
  }
  SignComboBoxItems.Add(NegativeIndicator);
  SignComboBoxItems.Emplace(PositiveIndicator);
  SignComboBox = SNew(STextComboBox)
                     .Font(FontInfo)
                     .ToolTipText(signTooltip)
                     .OptionsSource(&SignComboBoxItems)
                     .OnSelectionChanged(
                         this,
                         &CesiumDegreesMinutesSecondsEditor::SignChanged);
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
                // The 'degrees' symbol
                SNew(STextBlock)
                  .Text(FText::FromString(TEXT("\u00B0")))
                  .ToolTipText(FText::FromString("Degrees"))
            ]
            + SHorizontalBox::Slot().FillWidth(1.0)
            [
                MinutesSpinBox.ToSharedRef()
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(hPad, 0.0f)
            [
                // The 'minutes' symbol
                SNew(STextBlock)
                  .Text(FText::FromString(TEXT("\u2032")))
                  .ToolTipText(FText::FromString("Minutes"))
            ]
            + SHorizontalBox::Slot().FillWidth(1.0)
            [
                SecondsSpinBox.ToSharedRef()
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(hPad, 0.0f)
            [
                // The 'seconds' symbol
                SNew(STextBlock)
                  .Text(FText::FromString(TEXT("\u2033")))
                  .ToolTipText(FText::FromString("Seconds"))
            ]
            + SHorizontalBox::Slot().AutoWidth()
            [
                SignComboBox.ToSharedRef()
            ]
        ]
		];
  // clang-format on
}

double
CesiumDegreesMinutesSecondsEditor::GetDecimalDegreesFromProperty() const {
  double decimalDegrees;
  FPropertyAccess::Result AccessResult =
      DecimalDegreesHandle->GetValue(decimalDegrees);
  if (AccessResult == FPropertyAccess::Success) {
    return decimalDegrees;
  }

  // In theory, this should never happen if the actual property is a double. But
  // in practice it gets triggered when saving a level, for some reason. So, we
  // ignore it.
  return 0.0;
}

void CesiumDegreesMinutesSecondsEditor::SetDecimalDegreesOnProperty(
    double NewValue) {
  DecimalDegreesHandle->SetValue(NewValue);
  SignComboBox->SetSelectedItem(
      NewValue < 0 ? NegativeIndicator : PositiveIndicator);
}

int32 CesiumDegreesMinutesSecondsEditor::GetDegrees() const {
  double decimalDegrees = GetDecimalDegreesFromProperty();
  DMS dms = decimalDegreesToDms(decimalDegrees);
  return static_cast<int32>(dms.d);
}
void CesiumDegreesMinutesSecondsEditor::SetDegrees(int32 NewValue) {
  double decimalDegrees = GetDecimalDegreesFromProperty();
  DMS dms = decimalDegreesToDms(decimalDegrees);
  dms.d = NewValue;
  double newDecimalDegreesValue = dmsToDecimalDegrees(dms);
  SetDecimalDegreesOnProperty(newDecimalDegreesValue);
}

int32 CesiumDegreesMinutesSecondsEditor::GetMinutes() const {
  double decimalDegrees = GetDecimalDegreesFromProperty();
  DMS dms = decimalDegreesToDms(decimalDegrees);
  return static_cast<int32>(dms.m);
}
void CesiumDegreesMinutesSecondsEditor::SetMinutes(int32 NewValue) {
  double decimalDegrees = GetDecimalDegreesFromProperty();
  DMS dms = decimalDegreesToDms(decimalDegrees);
  dms.m = NewValue;
  double newDecimalDegreesValue = dmsToDecimalDegrees(dms);
  SetDecimalDegreesOnProperty(newDecimalDegreesValue);
}

double CesiumDegreesMinutesSecondsEditor::GetSeconds() const {
  double decimalDegrees = GetDecimalDegreesFromProperty();
  DMS dms = decimalDegreesToDms(decimalDegrees);
  return dms.s;
}
void CesiumDegreesMinutesSecondsEditor::SetSeconds(double NewValue) {
  double decimalDegrees = GetDecimalDegreesFromProperty();
  DMS dms = decimalDegreesToDms(decimalDegrees);
  dms.s = NewValue;
  double newDecimalDegreesValue = dmsToDecimalDegrees(dms);
  SetDecimalDegreesOnProperty(newDecimalDegreesValue);
}

void CesiumDegreesMinutesSecondsEditor::SignChanged(
    TSharedPtr<FString> StringItem,
    ESelectInfo::Type SelectInfo) {

  bool negative = false;
  if (StringItem.IsValid()) {
    negative = (StringItem == NegativeIndicator);
  }
  double decimalDegrees = GetDecimalDegreesFromProperty();
  DMS dms = decimalDegreesToDms(decimalDegrees);
  dms.negative = negative;
  double newDecimalDegreesValue = dmsToDecimalDegrees(dms);
  SetDecimalDegreesOnProperty(newDecimalDegreesValue);
}
