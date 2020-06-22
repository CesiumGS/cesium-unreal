#pragma once

#include <functional>
#include "TilesetExternals.h"

class UnrealTaskProcessor : public ITaskProcessor {
public:
    virtual void startTask(std::function<void()> f);
};