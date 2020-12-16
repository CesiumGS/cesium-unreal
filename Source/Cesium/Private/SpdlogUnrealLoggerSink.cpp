#include "SpdlogUnrealLoggerSink.h"

#include "CoreMinimal.h"

#include <iostream>
#include <memory>

DEFINE_LOG_CATEGORY(LogCesiumNative);

std::shared_ptr<spdlog::sinks::base_sink<std::mutex>> createDefaultSink() {
    auto sinkPtr = std::make_shared<spdlog_logger_sink>();

    // Set the pattern for the sink:
    // Hour, minutes, seconds, millseconds, thread ID, message
    sinkPtr->set_pattern("[%H:%M:%S:%e] [thread %-5t] : %v");
    return sinkPtr;
}

