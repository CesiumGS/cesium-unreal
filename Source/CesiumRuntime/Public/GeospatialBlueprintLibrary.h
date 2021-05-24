// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "CesiumGeoreference.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GeospatialBlueprintLibrary.generated.h"

/**
 * 
 */
UCLASS()
class CESIUMRUNTIME_API UGeospatialBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

  public:
  static ACesiumGeoreference* DefaultGeoreference;
  
public:

  /**
  * Computes the rotation matrix from the local East-North-Up to Unreal at the
  * specified Unreal relative world location (relative to the floating
  * origin). The returned transformation works in Unreal's left-handed
  * coordinate system.
  */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  static FMatrix InaccurateComputeEastNorthUpToUnreal(const FVector& Ue);
  
  /**
  * Computes the rotation matrix from the local East-North-Up to
  * Earth-Centered, Earth-Fixed (ECEF) at the specified ECEF location.
  */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  static FMatrix ComputeEastNorthUpToEcef(const FVector& Ecef);



private:
  void _setDefaultGeoreference();
};
