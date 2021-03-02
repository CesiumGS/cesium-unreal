// Copyright CesiumGS, Inc. and Contributors

#include "SpdlogUnrealLoggerSink.h"
#include "CoreMinimal.h"

DEFINE_LOG_CATEGORY(LogCesiumNative);

void SpdlogUnrealLoggerSink::sink_it_(const spdlog::details::log_msg& msg) {
    switch (msg.level) {
    case SPDLOG_LEVEL_TRACE:
        UE_LOG(LogCesiumNative, VeryVerbose, TEXT("%s"), *this->formatMessage(msg));
        break;
    case SPDLOG_LEVEL_DEBUG:
        UE_LOG(LogCesiumNative, Verbose, TEXT("%s"), *this->formatMessage(msg));
        break;
    case SPDLOG_LEVEL_INFO:
        UE_LOG(LogCesiumNative, Display, TEXT("%s"), *this->formatMessage(msg));
        break;
    case SPDLOG_LEVEL_WARN:
        UE_LOG(LogCesiumNative, Warning, TEXT("%s"), *this->formatMessage(msg));
        break;
    case SPDLOG_LEVEL_ERROR:
        UE_LOG(LogCesiumNative, Error, TEXT("%s"), *this->formatMessage(msg));
        break;
    case SPDLOG_LEVEL_CRITICAL:
        UE_LOG(LogCesiumNative, Fatal, TEXT("%s"), *this->formatMessage(msg));
        break;
    }
}

void SpdlogUnrealLoggerSink::flush_() {
    // Nothing to do here
}

FString SpdlogUnrealLoggerSink::formatMessage(const spdlog::details::log_msg& msg) const {
    // Frustratingly, spdlog::formatter isn't thread safe. So even though our sink
    // itself doesn't need to be protected by a mutex, the formatter does.
    // See https://github.com/gabime/spdlog/issues/897
    std::scoped_lock<std::mutex> lock(this->_formatMutex);

    spdlog::memory_buf_t formatted;
    this->formatter_->format(msg, formatted);
    return UTF8_TO_TCHAR(fmt::to_string(formatted).c_str());
}
