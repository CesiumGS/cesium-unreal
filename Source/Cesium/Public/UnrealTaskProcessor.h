#pragma once

#include "Cesium3DTiles/ITaskProcessor.h"

class UnrealTaskProcessor : public Cesium3DTiles::ITaskProcessor {
public:
    virtual void startTask(std::function<void()> f) override;
};