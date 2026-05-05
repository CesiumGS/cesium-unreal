// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "IPropertyTypeCustomization.h"
#include "PropertyEditorModule.h"

class IDetailCategoryBuilder;

/**
 * An implementation of the IDetailCustomization interface that customizes
 * the Details View of a FCesiumMetadataValue. It is registered in
 * FCesiumEditorModule::StartupModule.
 */
class FCesiumMetadataValueCustomization : public IPropertyTypeCustomization {
public:
  // Makes a new instance of this customization
  static TSharedRef<IPropertyTypeCustomization> MakeInstance();

  static void Register(FPropertyEditorModule& PropertyEditorModule);
  static void Unregister(FPropertyEditorModule& PropertyEditorModule);

  // IPropertyTypeCustomization interface
  virtual void CustomizeHeader(
      TSharedRef<IPropertyHandle> StructPropertyHandle,
      class FDetailWidgetRow& HeaderRow,
      IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
  virtual void CustomizeChildren(
      TSharedRef<class IPropertyHandle> StructPropertyHandle,
      class IDetailChildrenBuilder& StructBuilder,
      IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
  // End of IPropertyTypeCustomization interface

  static FName RegisteredLayoutName;
};
