// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#include "CesiumRuntimeSettingsCustomization.h"
#include "CesiumCustomization.h"
#include "CesiumRuntimeSettings.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"

FName FCesiumRuntimeSettingsCustomization::RegisteredLayoutName;

void FCesiumRuntimeSettingsCustomization::Register(
    FPropertyEditorModule& PropertyEditorModule) {
  RegisteredLayoutName = UCesiumRuntimeSettings::StaticClass()->GetFName();
  PropertyEditorModule.RegisterCustomClassLayout(
      RegisteredLayoutName,
      FOnGetDetailCustomizationInstance::CreateStatic(
          &FCesiumRuntimeSettingsCustomization::MakeInstance));
}

void FCesiumRuntimeSettingsCustomization::Unregister(
    FPropertyEditorModule& PropertyEditorModule) {
  PropertyEditorModule.UnregisterCustomClassLayout(RegisteredLayoutName);
}

TSharedRef<IDetailCustomization>
FCesiumRuntimeSettingsCustomization::MakeInstance() {
  return MakeShareable(new FCesiumRuntimeSettingsCustomization);
}

void FCesiumRuntimeSettingsCustomization::CustomizeDetails(
    IDetailLayoutBuilder& DetailBuilder) {
  IDetailCategoryBuilder& CacheCategory = DetailBuilder.EditCategory("Cache");

  UFunction* ClearFunction =
      UCesiumRuntimeSettings::StaticClass()->FindFunctionByName(
          GET_FUNCTION_NAME_CHECKED(UCesiumRuntimeSettings, ClearRequestCache));

  TSharedPtr<CesiumButtonGroup> ButtonGroup =
      CesiumCustomization::CreateButtonGroup();
  ButtonGroup->AddButtonForUFunction(
      ClearFunction,
      NSLOCTEXT("CesiumEditor", "ClearRequestCache", "Clear Request Cache"));
  ButtonGroup->Finish(DetailBuilder, CacheCategory);
}
