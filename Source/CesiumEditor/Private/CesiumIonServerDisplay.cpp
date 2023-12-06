// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumIonServerDisplay.h"
#include "CesiumEditor.h"
#include "CesiumIonServer.h"
#include "Editor.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Input/SEditableTextBox.h"

void CesiumIonServerDisplay::Construct(const FArguments& InArgs) {
  UCesiumIonServer* pServer = InArgs._Server;

  ChildSlot
      [SNew(SHorizontalBox) +
       SHorizontalBox::Slot()
           .AutoWidth()
           .VAlign(EVerticalAlignment::VAlign_Center)
           .Padding(5.0f)[SNew(STextBlock)
                              .Text(FText::FromString("Cesium ion Server:"))] +
       SHorizontalBox::Slot()
           .AutoWidth()
           .VAlign(EVerticalAlignment::VAlign_Center)
           .Padding(5.0f)[SNew(SEditableTextBox)
                              .IsEnabled(false)
                              .Padding(5.0f)
                              .Text(FText::FromString(pServer->DisplayName))] +
       SHorizontalBox::Slot()
           .AutoWidth()
           .VAlign(EVerticalAlignment::VAlign_Center)
           .Padding(5.0f)[PropertyCustomizationHelpers::MakeBrowseButton(
               FSimpleDelegate::
                   CreateSP(this, &CesiumIonServerDisplay::OnBrowseForServer),
               FText::FromString(
                   "Show this Cesium ion Server in the Content Browser."),
               true,
               false)]];
}

void CesiumIonServerDisplay::OnBrowseForServer() {
  TArray<UObject*> Objects;
  Objects.Add(FCesiumEditorModule::serverManager().GetCurrentServer());
  GEditor->SyncBrowserToObjects(Objects);
}
