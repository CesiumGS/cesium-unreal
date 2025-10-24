// // Copyright 2020-2025 CesiumGS, Inc. and Contributors

#include "CesiumMetadataValueCustomization.h"
#include "CesiumFeaturesMetadataDescription.h"
#include "CesiumMetadataValue.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"

FName FCesiumMetadataValueCustomization::RegisteredLayoutName;

TSharedRef<IPropertyTypeCustomization>
FCesiumMetadataValueCustomization::MakeInstance() {
  return MakeShareable(new FCesiumMetadataValueCustomization);
}

void FCesiumMetadataValueCustomization::Register(
    FPropertyEditorModule& PropertyEditorModule) {

  RegisteredLayoutName = FCesiumMetadataValue::StaticStruct()->GetFName();

  PropertyEditorModule.RegisterCustomPropertyTypeLayout(
      RegisteredLayoutName,
      FOnGetPropertyTypeCustomizationInstance::CreateStatic(
          &FCesiumMetadataValueCustomization::MakeInstance));
}

void FCesiumMetadataValueCustomization::Unregister(
    FPropertyEditorModule& PropertyEditorModule) {
  PropertyEditorModule.UnregisterCustomPropertyTypeLayout(RegisteredLayoutName);
}

namespace {
const FCesiumMetadataValue*
getValue(const TSharedRef<IPropertyHandle>& PropertyHandle) {
  TArray<void*> rawDataPointers;
  PropertyHandle->AccessRawData(rawDataPointers);

  if (rawDataPointers.Num() != 1)
    return nullptr;

  return (FCesiumMetadataValue*)rawDataPointers[0];
}
} // namespace

void FCesiumMetadataValueCustomization::CustomizeHeader(
    TSharedRef<IPropertyHandle> PropertyHandle,
    class FDetailWidgetRow& HeaderRow,
    IPropertyTypeCustomizationUtils& CustomizationUtils) {
  const FCesiumMetadataValue* pValue = getValue(PropertyHandle);
  if (!pValue) {
    HeaderRow.ValueContent()[SNew(SBox).Padding(FMargin(
        5.f,
        5.f,
        0.f,
        5.f))[SNew(STextBlock)
                  .Text(FText::FromString(UCesiumMetadataValueBlueprintLibrary::
                                              GetString(*pValue, FString())))]];
  }
}

void FCesiumMetadataValueCustomization::CustomizeChildren(
    TSharedRef<class IPropertyHandle> PropertyHandle,
    class IDetailChildrenBuilder& ChildrenBuilder,
    IPropertyTypeCustomizationUtils& CustomizationUtils) {}
