// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumAsync/ITaskProcessor.h"

class CESIUM_API UnrealTaskProcessor : public CesiumAsync::ITaskProcessor {
public:
    virtual void startTask(std::function<void()> f) override;
};