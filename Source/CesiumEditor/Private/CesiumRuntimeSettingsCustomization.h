// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "IDetailCustomization.h"
#include "PropertyEditorModule.h"

/**
 * An implementation of IDetailCustomization that adds interactive buttons
 * (e.g. "Clear Request Cache") to the UCesiumRuntimeSettings details view
 * shown in Project Settings -> Plugins -> Cesium.
 */
class FCesiumRuntimeSettingsCustomization : public IDetailCustomization {
public:
  virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

  static void Register(FPropertyEditorModule& PropertyEditorModule);
  static void Unregister(FPropertyEditorModule& PropertyEditorModule);

  static TSharedRef<IDetailCustomization> MakeInstance();

  static FName RegisteredLayoutName;
};
