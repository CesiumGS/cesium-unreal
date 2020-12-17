#include "UnrealConversions.h"

FString utf8_to_wstr(const std::string& utf8) {
	return UTF8_TO_TCHAR(utf8.c_str());
}

std::string wstr_to_utf8(const FString& utf16) {
	return TCHAR_TO_UTF8(*utf16);
}
