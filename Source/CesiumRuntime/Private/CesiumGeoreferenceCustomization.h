// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "IDetailCustomization.h"
#include "Types/SlateEnums.h"
#include "Widgets/Input/SSpinBox.h"

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

  double GetOriginLongitudeFromProperty() const;
  void SetOriginLongitudeOnProperty( double NewValue);

  int32 GetOriginLongitudeDegrees() const;
  void SetOriginLongitudeDegrees( int32 NewValue);

  int32 GetOriginLongitudeMinutes() const;
  void SetOriginLongitudeMinutes( int32 NewValue);

  double GetOriginLongitudeSeconds() const;
  void SetOriginLongitudeSeconds( double NewValue);

};
