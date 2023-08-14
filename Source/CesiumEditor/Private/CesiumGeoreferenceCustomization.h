// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

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
  FReply
  OnPlaceGeoreferenceOriginHere(TWeakObjectPtr<UFunction> WeakFunctionPtr);

  TSharedPtr<CesiumDegreesMinutesSecondsEditor> LongitudeEditor;
  TSharedPtr<CesiumDegreesMinutesSecondsEditor> LatitudeEditor;
  TArray<TWeakObjectPtr<UObject>> SelectedObjects;
};
