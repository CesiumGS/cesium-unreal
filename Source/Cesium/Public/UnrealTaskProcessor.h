#pragma once

#include <functional>
#include "Cesium3DTilesetExternals.h"

class UnrealTaskProcessor : public ITaskProcessor {
public:
    virtual void startTask(std::function<void()> f);
};