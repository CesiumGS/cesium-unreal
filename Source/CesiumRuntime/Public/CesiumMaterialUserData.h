#pragma once

#include "Engine/AssetUserData.h"
#include "CesiumMaterialUserData.generated.h"

UCLASS()
class CESIUMRUNTIME_API UCesiumMaterialUserData : public UAssetUserData {
  GENERATED_BODY()

public:
  virtual void PostEditChangeOwner() override;

  UPROPERTY()
  TArray<FString> LayerNames;
};
