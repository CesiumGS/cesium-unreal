// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "UnrealTaskProcessor.h"
#include "Async/Async.h"
#include "Misc/QueuedThreadPool.h"

UnrealTaskProcessor::UnrealTaskProcessor()
    : _pThreadPool(FQueuedThreadPool::Allocate()) {
  _pThreadPool->Create(12, 5 * 1024 * 1024, TPri_SlightlyBelowNormal, TEXT("CesiumThreadPool"));
}

UnrealTaskProcessor::~UnrealTaskProcessor() {
  _pThreadPool->Destroy();
}

void UnrealTaskProcessor::startTask(std::function<void()> f) {
  //AsyncTask(ENamedThreads::Type::AnyBackgroundThreadNormalTask, [f]() { f(); });
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::StartTask)
  AsyncPool(
      *_pThreadPool,
      [f]() {
        TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::PoolTask)
        f();
      },
      []() {});
}
