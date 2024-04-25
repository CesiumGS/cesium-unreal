// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "IDetailCustomization.h"
#include "IDetailPropertyRow.h"
#include "Types/SlateEnums.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/STextComboBox.h"

/**
 * A class that allows configuring a Details View row for a
 * latitude or longitude property.
 *
 * Latitude and longitude properties are often computed with doubles
 * representing decimal-point degrees. This Details View row will show the
 * property with an additional Degree-Minutes-Seconds (DMS) view for easier
 * usability and editing.
 *
 * See FCesiumGeoreferenceCustomization::CustomizeDetails for
 * an example of how to use this class.
 */
class CesiumDegreesMinutesSecondsEditor
    : public TSharedFromThis<CesiumDegreesMinutesSecondsEditor> {

public:
  /**
   * Creates a new instance.
   *
   * The given property handle must be a handle to a 'double'
   * property!
   *
   * @param InputDecimalDegreesHandle The property handle for the
   * decimal degrees property
   * @param InputIsLongitude Whether the edited property is a
   * longitude (as opposed to a latitude) property
   */
  CesiumDegreesMinutesSecondsEditor(
      TSharedPtr<class IPropertyHandle> InputDecimalDegreesHandle,
      bool InputIsLongitude);

  /**
   * Populates the given Details View row with the default
   * editor (a SSpinBox for the value), as well as the
   * spin boxes and dropdowns for the DMS editing.
   *
   * @param Row The Details View row
   */
  void PopulateRow(IDetailPropertyRow& Row);

private:
  TSharedPtr<class IPropertyHandle> DecimalDegreesHandle;
  bool IsLongitude;

  TSharedPtr<SSpinBox<double>> DecimalDegreesSpinBox;
  TSharedPtr<SSpinBox<int32>> DegreesSpinBox;
  TSharedPtr<SSpinBox<int32>> MinutesSpinBox;
  TSharedPtr<SSpinBox<double>> SecondsSpinBox;

  TSharedPtr<FString> NegativeIndicator;
  TSharedPtr<FString> PositiveIndicator;

  TArray<TSharedPtr<FString>> SignComboBoxItems;
  TSharedPtr<STextComboBox> SignComboBox;

  double GetDecimalDegreesFromProperty() const;
  void SetDecimalDegreesOnProperty(double NewValue);

  int32 GetDegrees() const;
  void SetDegrees(int32 NewValue);

  int32 GetMinutes() const;
  void SetMinutes(int32 NewValue);

  double GetSeconds() const;
  void SetSeconds(double NewValue);

  void
  SignChanged(TSharedPtr<FString> StringItem, ESelectInfo::Type SelectInfo);
};
