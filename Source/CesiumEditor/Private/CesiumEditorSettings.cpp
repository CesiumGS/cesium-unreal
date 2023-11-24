// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumEditorSettings.h"
#include "CesiumSourceControl.h"

UCesiumEditorSettings::UCesiumEditorSettings(
    const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {}

void UCesiumEditorSettings::Save() {
  CesiumSourceControl::PromptToCheckoutConfigFile(
      this->GetClass()->GetConfigName());
  this->Modify();
  this->SaveConfig();
}
