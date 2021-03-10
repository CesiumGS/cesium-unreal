// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "UnrealConversions.h"
#include <codecvt>
#include <locale>

FString utf8_to_wstr(const std::string& utf8) {
  static thread_local std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>
      wcu8;
  return FString(wcu8.from_bytes(utf8).c_str());
}

std::string wstr_to_utf8(const FString& utf16) {
  static thread_local std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>
      wcu8;
  return wcu8.to_bytes(*utf16);
}
