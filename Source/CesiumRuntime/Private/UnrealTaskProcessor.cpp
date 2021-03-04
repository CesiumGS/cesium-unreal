// Copyright CesiumGS, Inc. and Contributors

#include "UnrealTaskProcessor.h"
#include "Async/Async.h"

void UnrealTaskProcessor::startTask(std::function<void()> f) {
    AsyncTask(ENamedThreads::Type::AnyThread, [f]() {
        f();
    });
}
