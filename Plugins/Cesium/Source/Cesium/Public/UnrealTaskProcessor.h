#pragma once

#include <functional>
#include "Cesium3DTiles/TilesetExternals.h"

class UnrealTaskProcessor : public Cesium3DTiles::ITaskProcessor {
public:
    virtual void startTask(std::function<void()> f);
};