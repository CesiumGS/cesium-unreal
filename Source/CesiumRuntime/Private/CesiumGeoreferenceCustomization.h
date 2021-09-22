// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#if WITH_EDITOR

#include "CesiumDegreesMinutesSecondsEditor.h"
#include "IDetailCustomization.h"

/**
 * An implementation of the IDetailCustomization interface that customizes
 * the Details View of a CesiumGeoreference.
 *
 * It is registered in FCesiumRuntimeModule::StartupModule, and will
 * use a CesiumDegreesMinutesSecondsEditor for the OriginLatitude
 * and OriginLongitude properties.
 */
class FCesiumGeoreferenceCustomization : public IDetailCustomization {
public:
  virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

  static TSharedRef<IDetailCustomization> MakeInstance();

private:
  TSharedPtr<CesiumDegreesMinutesSecondsEditor> LongitudeEditor;
  TSharedPtr<CesiumDegreesMinutesSecondsEditor> LatitudeEditor;
};

#endif // WITH_EDITOR
