// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumRuntimeSettings.h"
#include "CesiumRuntime.h"

UCesiumRuntimeSettings::UCesiumRuntimeSettings(
    const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
  CategoryName = FName(TEXT("Plugins"));
}

void UCesiumRuntimeSettings::ClearRequestCache() {
  getCacheDatabase()->clearAll();
  UE_LOG(LogCesium, Info, TEXT("Cesium request cache cleared."));
}
