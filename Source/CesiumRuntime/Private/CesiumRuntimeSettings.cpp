// Copyright 2020-2026 CesiumGS, Inc. and Contributors

#include "CesiumRuntimeSettings.h"
#include "CesiumAsync/SqliteCache.h"
#include "CesiumRuntime.h"

UCesiumRuntimeSettings::UCesiumRuntimeSettings(
    const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
  CategoryName = FName(TEXT("Plugins"));
}

/*static*/ void UCesiumRuntimeSettings::ClearRequestCache() {
  getCacheDatabase()->clearAll();
  UE_LOG(LogCesium, Display, TEXT("Cesium request cache cleared."));
}
