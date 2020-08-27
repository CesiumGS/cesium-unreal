#pragma warning(disable:4583)
#pragma warning(disable:4582)

#include "UnrealTaskProcessor.h"
#include "Async/Async.h"

void UnrealTaskProcessor::startTask(std::function<void()> f) {
    AsyncTask(ENamedThreads::Type::AnyThread, [f]() {
        f();
    });
}
