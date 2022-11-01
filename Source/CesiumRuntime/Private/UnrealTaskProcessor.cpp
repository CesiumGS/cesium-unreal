// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "UnrealTaskProcessor.h"
#include "Async/Async.h"
#include "Misc/QueuedThreadPool.h"

void UnrealTaskProcessor::startTask(std::function<void()> f) {
  AsyncTask(ENamedThreads::Type::AnyBackgroundThreadNormalTask, [f]() {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::AsyncTask)
    f();
  });
}
