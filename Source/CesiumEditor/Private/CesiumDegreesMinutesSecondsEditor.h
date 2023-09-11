// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "IDetailCustomization.h"
#include "IDetailPropertyRow.h"
#include "Types/SlateEnums.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/STextComboBox.h"

/**
 * A class that allows configuring a Details View row that shows
 * a latitude or longitude.
 *
 * Assuming that the property was given as a (double) property
 * that contains the decimal degrees, the Details View row
 * that is created with this class shows the value additionally
 * in a DMS (Degree-Minutes-Seconds) view.
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
   * @param InputDecimalDegreesHandle The property hande for the
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
