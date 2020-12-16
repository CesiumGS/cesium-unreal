#pragma once

#include "CoreMinimal.h"

#include <memory>
#include <mutex>

#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/null_mutex.h>

DECLARE_LOG_CATEGORY_EXTERN(LogCesiumNative, Log, All);

/**
 * @brief Internal implementation of a spdlog sink that forwards the messages
 * to Unreal log macros.
 */
class spdlog_logger_sink : public spdlog::sinks::base_sink<std::mutex>
{
protected:
	void sink_it_(const spdlog::details::log_msg& msg) override {

        spdlog::memory_buf_t formatted;
        spdlog_logger_sink::formatter_->format(msg, formatted);
        std::string message = fmt::to_string(formatted);

        switch (msg.level) {
        case SPDLOG_LEVEL_TRACE:
            UE_LOG(LogCesiumNative, VeryVerbose, TEXT("%s"), UTF8_TO_TCHAR(message.c_str()));
            return;
        case SPDLOG_LEVEL_DEBUG:
            UE_LOG(LogCesiumNative, Verbose, TEXT("%s"), UTF8_TO_TCHAR(message.c_str()));
            return;
        case SPDLOG_LEVEL_INFO:
            UE_LOG(LogCesiumNative, Display, TEXT("%s"), UTF8_TO_TCHAR(message.c_str()));
            return;
        case SPDLOG_LEVEL_WARN:
            UE_LOG(LogCesiumNative, Warning, TEXT("%s"), UTF8_TO_TCHAR(message.c_str()));
            return;
        case SPDLOG_LEVEL_ERROR:
            //UE_LOG(LogCesiumNative, Error, TEXT("%s"), UTF8_TO_TCHAR(message.c_str()));
            return;
        case SPDLOG_LEVEL_CRITICAL:
            UE_LOG(LogCesiumNative, Fatal, TEXT("%s"), UTF8_TO_TCHAR(message.c_str()));
            return;
        }
        UE_LOG(LogCesiumNative, Warning, TEXT("%s"), UTF8_TO_TCHAR(message.c_str()));
    }

	void flush_() override {
		// Nothing to do here
	}

};

std::shared_ptr<spdlog::sinks::base_sink<std::mutex>> createDefaultSink();
