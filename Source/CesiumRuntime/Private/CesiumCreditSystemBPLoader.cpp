// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumCreditSystemBPLoader.h"
#include "UObject/ConstructorHelpers.h"

UCesiumCreditSystemBPLoader::UCesiumCreditSystemBPLoader()
    : CesiumCreditSystemBP(
          ConstructorHelpers::FObjectFinder<UClass>(
              TEXT(
                  "Class'/CesiumForUnreal/CesiumCreditSystemBP.CesiumCreditSystemBP_C'"))
              .Object) {}
