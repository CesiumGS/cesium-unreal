// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once
#include "Modules/ModuleManager.h"
#include "CesiumModuleListener.h"

class CesiumGeoreferenceModeTool : public ICesiumModuleListener {
public:
    virtual void OnStartupModule() override;
    virtual void OnShutdownModule() override;

    virtual ~CesiumGeoreferenceModeTool() {}

private:
    static TSharedPtr<class FSlateStyleSet> StyleSet;

    void RegisterStyleSet();
    void UnregisterStyleSet();

    void RegisterEditorMode();
    void UnregisterEditorMode();
};