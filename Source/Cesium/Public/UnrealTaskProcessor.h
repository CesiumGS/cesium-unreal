#pragma once

#include "CesiumAsync/ITaskProcessor.h"

class UnrealTaskProcessor : public CesiumAsync::ITaskProcessor {
public:
    virtual void startTask(std::function<void()> f) override;
};