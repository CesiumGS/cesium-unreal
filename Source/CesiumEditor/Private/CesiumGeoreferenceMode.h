// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Editor.h"
#include "EdMode.h"

class FCesiumGeoreferenceMode : public FEdMode {
public:
    const static FEditorModeID EM_CesiumGeoreferenceMode;

    virtual void Enter() override;
    virtual void Exit() override;
};