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
  TSharedPtr< class IPropertyHandle > OriginLongitudeHandle;

  TSharedPtr<SSpinBox<double>> OriginLongitudeSpinBox;
  TSharedPtr<SSpinBox<int32>> OriginLongitudeDegreesSpinBox;
  TSharedPtr<SSpinBox<int32>> OriginLongitudeMinutesSpinBox;
  TSharedPtr<SSpinBox<double>> OriginLongitudeSecondsSpinBox;

  TSharedPtr<FString> NegativeIndicator;
  TSharedPtr<FString> PositiveIndicator;

  TArray<TSharedPtr<FString>> SignComboBoxItems;
  TSharedPtr<STextComboBox> SignComboBox;

  double GetOriginLongitudeFromProperty() const;
  void SetOriginLongitudeOnProperty( double NewValue);

  int32 GetOriginLongitudeDegrees() const;
  void SetOriginLongitudeDegrees( int32 NewValue);

  int32 GetOriginLongitudeMinutes() const;
  void SetOriginLongitudeMinutes( int32 NewValue);

  double GetOriginLongitudeSeconds() const;
  void SetOriginLongitudeSeconds( double NewValue);

  void SignChanged(TSharedPtr<FString> StringItem, ESelectInfo::Type SelectInfo);
};
