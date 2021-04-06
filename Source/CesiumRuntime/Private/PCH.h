// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include "CoreMinimal.h"

#ifdef _MSC_VER
// Workaround for an unhelpful warning (that gets treated as an error) in
// VS2017. See https://github.com/akrzemi1/Optional/issues/57 and
// https://answers.unrealengine.com/questions/607946/anonymous-union-with-none-trivial-type.html
#ifdef _MSC_VER
#if _MSC_VER < 1920
#pragma warning(push)
#pragma warning(disable : 4583)
#pragma warning(disable : 4582)
#include <optional>
#include <variant>
#pragma warning(pop)
#endif
#endif // ifdef _MSC_VER
