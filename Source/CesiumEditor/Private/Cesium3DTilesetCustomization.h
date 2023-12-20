// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "IDetailCustomization.h"

class IDetailCategoryBuilder;

/**
 * An implementation of the IDetailCustomization interface that customizes
 * the Details View of a Cesium3DTileset. It is registered in
 * FCesiumEditorModule::StartupModule.
 */
class FCesium3DTilesetCustomization : public IDetailCustomization {
public:
  virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

  static void Register(FPropertyEditorModule& PropertyEditorModule);
  static void Unregister(FPropertyEditorModule& PropertyEditorModule);

  static TSharedRef<IDetailCustomization> MakeInstance();

  static void SortCustomDetailsCategories(
      const TMap<FName, IDetailCategoryBuilder*>& AllCategoryMap);
};
