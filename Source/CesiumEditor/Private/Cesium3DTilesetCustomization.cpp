// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "Cesium3DTilesetCustomization.h"
#include "Cesium3DTileset.h"
#include "CesiumCustomization.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"

void FCesium3DTilesetCustomization::Register(
    FPropertyEditorModule& PropertyEditorModule) {
  PropertyEditorModule.RegisterCustomClassLayout(
      ACesium3DTileset::StaticClass()->GetFName(),
      FOnGetDetailCustomizationInstance::CreateStatic(
          &FCesium3DTilesetCustomization::MakeInstance));
}

void FCesium3DTilesetCustomization::Unregister(
    FPropertyEditorModule& PropertyEditorModule) {
  PropertyEditorModule.UnregisterCustomClassLayout(
      ACesium3DTileset::StaticClass()->GetFName());
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
