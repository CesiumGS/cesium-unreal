#pragma once

#include "CesiumCreditSystemBPLoader.generated.h"

UCLASS()
class UCesiumCreditSystemBPLoader : public UObject {
  GENERATED_BODY()

public:
  UCesiumCreditSystemBPLoader();

  UPROPERTY()
  UClass* CesiumCreditSystemBP;
};
