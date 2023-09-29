// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumDegreesMinutesSecondsEditor.h"
#include "IDetailCustomization.h"

/**
 * An implementation of the IDetailCustomization interface that customizes
 * the Details View of a CesiumGeoreference. It is registered in
 * FCesiumEditorModule::StartupModule.
 */
class FCesiumGeoreferenceCustomization : public IDetailCustomization {
public:
  virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

  static void Register(FPropertyEditorModule& PropertyEditorModule);
  static void Unregister(FPropertyEditorModule& PropertyEditorModule);

  static TSharedRef<IDetailCustomization> MakeInstance();

private:
  TSharedPtr<CesiumDegreesMinutesSecondsEditor> LongitudeEditor;
  TSharedPtr<CesiumDegreesMinutesSecondsEditor> LatitudeEditor;
};
