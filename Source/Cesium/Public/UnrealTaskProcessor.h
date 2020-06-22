#pragma once

#include <functional>
#include "TilesetExternals.h"

class UnrealTaskProcessor : public Cesium3DTiles::ITaskProcessor {
public:
    virtual void startTask(std::function<void()> f);
};