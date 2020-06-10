#pragma once

#include <string>
#include "CoreMinimal.h"

FString utf8_to_wstr(const std::string& utf8);
std::string wstr_to_utf8(const FString& utf16);
