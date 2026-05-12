// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "Cesium3DTilesetCustomization.h"
#include "Cesium3DTileset.h"
#include "CesiumCustomization.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"

FName FCesium3DTilesetCustomization::RegisteredLayoutName;

void FCesium3DTilesetCustomization::Register(
    FPropertyEditorModule& PropertyEditorModule) {

  RegisteredLayoutName = ACesium3DTileset::StaticClass()->GetFName();

  PropertyEditorModule.RegisterCustomClassLayout(
      RegisteredLayoutName,
      FOnGetDetailCustomizationInstance::CreateStatic(
          &FCesium3DTilesetCustomization::MakeInstance));
}

void FCesium3DTilesetCustomization::Unregister(
    FPropertyEditorModule& PropertyEditorModule) {
  PropertyEditorModule.UnregisterCustomClassLayout(RegisteredLayoutName);
}

TSharedRef<IDetailCustomization> FCesium3DTilesetCustomization::MakeInstance() {
  return MakeShareable(new FCesium3DTilesetCustomization);
}

void FCesium3DTilesetCustomization::CustomizeDetails(
    IDetailLayoutBuilder& DetailBuilder) {
  DetailBuilder.SortCategories(&SortCustomDetailsCategories);
}

void FCesium3DTilesetCustomization::SortCustomDetailsCategories(
    const TMap<FName, IDetailCategoryBuilder*>& AllCategoryMap) {
  (*AllCategoryMap.Find(FName("TransformCommon")))->SetSortOrder(0);
  (*AllCategoryMap.Find(FName("Cesium")))->SetSortOrder(1);
  (*AllCategoryMap.Find(FName("Rendering")))->SetSortOrder(2);
}
