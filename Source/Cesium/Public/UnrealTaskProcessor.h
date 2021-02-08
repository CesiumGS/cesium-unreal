#pragma once

#include "CesiumAsync/ITaskProcessor.h"

class CESIUM_API UnrealTaskProcessor : public CesiumAsync::ITaskProcessor {
public:
    virtual void startTask(std::function<void()> f) override;
};