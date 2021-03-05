// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/null_mutex.h>
#include <memory>
#include <mutex>

DECLARE_LOG_CATEGORY_EXTERN(LogCesiumNative, Log, All);

/**
 * @brief Internal implementation of a spdlog sink that forwards the messages
 * to Unreal log macros.
 */
class SpdlogUnrealLoggerSink : public spdlog::sinks::base_sink<spdlog::details::null_mutex>
{
protected:
    virtual void sink_it_(const spdlog::details::log_msg& msg) override;
    virtual void flush_() override;

private:
    FString formatMessage(const spdlog::details::log_msg& msg) const;

    mutable std::mutex _formatMutex;
};
