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
FString getValueAsString(const TSharedRef<IPropertyHandle>& PropertyHandle) {
  const FString defaultReturn = FString("(no value)");

  TArray<void*> rawDataPointers;
  PropertyHandle->AccessRawData(rawDataPointers);

  if (rawDataPointers.Num() != 1)
    return defaultReturn;

  const FCesiumMetadataValue* pValue =
      reinterpret_cast<FCesiumMetadataValue*>(rawDataPointers[0]);

  if (pValue) {
    return UCesiumMetadataValueBlueprintLibrary::GetString(
        *pValue,
        defaultReturn);
  }

  return defaultReturn;
}
} // namespace

void FCesiumMetadataValueCustomization::CustomizeHeader(
    TSharedRef<IPropertyHandle> PropertyHandle,
    class FDetailWidgetRow& HeaderRow,
    IPropertyTypeCustomizationUtils& CustomizationUtils) {
  HeaderRow
      .NameContent()[SNew(STextBlock)
                         .Text(FText::FromString("Value"))
                         .Font(CustomizationUtils.GetRegularFont())]
      .ValueContent()[SNew(STextBlock)
                          .Text_Lambda([PropertyHandle]() {
                            return FText::FromString(
                                getValueAsString(PropertyHandle));
                          })
                          .Font(CustomizationUtils.GetRegularFont())];
}

void FCesiumMetadataValueCustomization::CustomizeChildren(
    TSharedRef<class IPropertyHandle> PropertyHandle,
    class IDetailChildrenBuilder& ChildrenBuilder,
    IPropertyTypeCustomizationUtils& CustomizationUtils) {}
