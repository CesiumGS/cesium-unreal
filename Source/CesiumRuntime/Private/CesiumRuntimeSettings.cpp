// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumRuntimeSettings.h"

UCesiumRuntimeSettings::UCesiumRuntimeSettings(
    const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
  CategoryName = FName(TEXT("Plugins"));
}
