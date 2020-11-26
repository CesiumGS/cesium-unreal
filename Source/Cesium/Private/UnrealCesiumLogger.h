#pragma once

#include <string>

#include "CoreMinimal.h"

#include "Cesium3DTiles/ILogger.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCesiumNative, Log, All);

class UnrealCesiumLogger : public Cesium3DTiles::ILogger {
public:
	void log(Cesium3DTiles::ILogger::Level level, const std::string& message) override;
};
