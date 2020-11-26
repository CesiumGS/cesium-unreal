
#include "UnrealCesiumLogger.h"

#include <iostream>

#include "CoreMinimal.h"


DEFINE_LOG_CATEGORY(LogCesiumNative);

void UnrealCesiumLogger::log(Level level, const std::string& message) {

    // TODO Figure out what "VeryVerbose" is, exactly, and store
    // it in a variable (if this is possible...)
    switch (level) {
    case Cesium3DTiles::ILogger::Level::CESIUM_LOG_LEVEL_TRACE:
        UE_LOG(LogCesiumNative, VeryVerbose, TEXT("%s"), UTF8_TO_TCHAR(message.c_str()));
        return;
    case Cesium3DTiles::ILogger::Level::CESIUM_LOG_LEVEL_DEBUG:
        UE_LOG(LogCesiumNative, Verbose, TEXT("%s"), UTF8_TO_TCHAR(message.c_str()));
        return;
    case Cesium3DTiles::ILogger::Level::CESIUM_LOG_LEVEL_INFO:
        UE_LOG(LogCesiumNative, Display, TEXT("%s"), UTF8_TO_TCHAR(message.c_str()));
        return;
    case Cesium3DTiles::ILogger::Level::CESIUM_LOG_LEVEL_WARN:
        UE_LOG(LogCesiumNative, Warning, TEXT("%s"), UTF8_TO_TCHAR(message.c_str()));
        return;
    case Cesium3DTiles::ILogger::Level::CESIUM_LOG_LEVEL_ERROR:
        UE_LOG(LogCesiumNative, Error, TEXT("%s"), UTF8_TO_TCHAR(message.c_str()));
        return;
    case Cesium3DTiles::ILogger::Level::CESIUM_LOG_LEVEL_CRITICAL:
        UE_LOG(LogCesiumNative, Fatal, TEXT("%s"), UTF8_TO_TCHAR(message.c_str()));
        return;
    }
    UE_LOG(LogCesiumNative, Warning, TEXT("%s"), UTF8_TO_TCHAR(message.c_str()));
}
