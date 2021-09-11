// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "IDetailCustomization.h"
#include "Types/SlateEnums.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/STextComboBox.h"

class FCesiumGeoreferenceCustomization: public IDetailCustomization
{
public:
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

    static TSharedRef< IDetailCustomization > MakeInstance();


private:
  TSharedPtr< class IPropertyHandle > DecimalDegreesHandle;

  TSharedPtr<SSpinBox<double>> DecimalDegreesSpinBox;
  TSharedPtr<SSpinBox<int32>> DegreesSpinBox;
  TSharedPtr<SSpinBox<int32>> MinutesSpinBox;
  TSharedPtr<SSpinBox<double>> SecondsSpinBox;

  TSharedPtr<FString> NegativeIndicator;
  TSharedPtr<FString> PositiveIndicator;

  TArray<TSharedPtr<FString>> SignComboBoxItems;
  TSharedPtr<STextComboBox> SignComboBox;

  double GetDecimalDegreesFromProperty() const;
  void SetDecimalDegreesOnProperty( double NewValue);

  int32 GetDegrees() const;
  void SetDegrees( int32 NewValue);

  int32 GetMinutes() const;
  void SetMinutes( int32 NewValue);

  double GetSeconds() const;
  void SetSeconds( double NewValue);

  void SignChanged(TSharedPtr<FString> StringItem, ESelectInfo::Type SelectInfo);
};
