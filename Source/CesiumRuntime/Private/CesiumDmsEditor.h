// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "IDetailCustomization.h"
#include "IDetailPropertyRow.h"
#include "Types/SlateEnums.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/STextComboBox.h"

class CesiumDmsEditor : public TSharedFromThis<CesiumDmsEditor> {

public:
  CesiumDmsEditor(
      TSharedPtr<class IPropertyHandle> InputDecimalDegreesHandle,
      bool InputIsLongitude);

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
