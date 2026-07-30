// Copyright 2020-2026 CesiumGS, Inc. and Contributors

#pragma once

#include "Containers/UnrealString.h"

class CesiumSourceControl {
public:
  static void PromptToCheckoutConfigFile(const FString& RelativeConfigFilePath);
};
