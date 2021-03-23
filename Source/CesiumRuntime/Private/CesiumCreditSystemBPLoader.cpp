#include "CesiumCreditSystemBPLoader.h"
#include "UObject/ConstructorHelpers.h"

UCesiumCreditSystemBPLoader::UCesiumCreditSystemBPLoader()
    : CesiumCreditSystemBP(
          ConstructorHelpers::FObjectFinder<UClass>(
              TEXT(
                  "Class'/CesiumForUnreal/CesiumCreditSystemBP.CesiumCreditSystemBP_C'"))
              .Object) {}
