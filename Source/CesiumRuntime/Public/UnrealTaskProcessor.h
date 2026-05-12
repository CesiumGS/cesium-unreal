// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumAsync/ITaskProcessor.h"
#include "HAL/Platform.h"

class CESIUMRUNTIME_API UnrealTaskProcessor
    : public CesiumAsync::ITaskProcessor {
public:
  virtual void startTask(std::function<void()> f) override;
};
