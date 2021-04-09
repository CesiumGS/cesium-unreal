// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once
#include "CesiumGeoreferenceMode.h"
#include "CesiumGeoreferenceModeWidget.h"
#include "EditorModeManager.h"
#include "Toolkits/BaseToolkit.h"

class FCesiumGeoreferenceModeToolkit : public FModeToolkit {
public:
  FCesiumGeoreferenceModeToolkit() {
    SAssignNew(CesiumGeoreferenceModeWidget, SCesiumGeoreferenceModeWidget);
  }

  virtual FName GetToolkitFName() const override {
    return FName("CesiumGeoreferenceMode");
  }

  virtual FText GetBaseToolkitName() const override {
    return NSLOCTEXT("BuilderModeToolkit", "DisplayName", "Builder");
  }

  virtual class FEdMode* GetEditorMode() const override {
    return GLevelEditorModeTools().GetActiveMode(
        FCesiumGeoreferenceMode::EM_CesiumGeoreferenceMode);
  }

  virtual TSharedPtr<class SWidget> GetInlineContent() const override {
    return CesiumGeoreferenceModeWidget;
  }

private:
  TSharedPtr<SCesiumGeoreferenceModeWidget> CesiumGeoreferenceModeWidget;
};