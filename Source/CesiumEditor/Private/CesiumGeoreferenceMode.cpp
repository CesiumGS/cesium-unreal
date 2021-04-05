// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGeoreferenceMode.h"

#include "Editor/UnrealEd/Public/Toolkits/ToolkitManager.h"
#include "ScopedTransaction.h"
#include "CesiumEditor.h"
#include "CesiumGeoreferenceModeToolkit.h"

const FEditorModeID FCesiumGeoreferenceMode::EM_CesiumGeoreferenceMode(
    TEXT("EM_CesiumGeoreferenceMode"));

void FCesiumGeoreferenceMode::Enter() {
    FEdMode::Enter();

    if (!Toolkit.IsValid()) {
        Toolkit = MakeShareable(new FCesiumGeoreferenceModeToolkit);
        Toolkit->Init(Owner->GetToolkitHost());
    }
}

void FCesiumGeoreferenceMode::Exit() {
    FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
    Toolkit.Reset();
    
    FEdMode::Exit();
}
