// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include <glm/mat3x3.hpp>
#include "GeospatialLibrary.generated.h"

/**
 * 
 */
UCLASS()
class CESIUMRUNTIME_API UGeospatialLibrary : public UObject {
  GENERATED_BODY()

  public:

  /**
  * Computes the rotation matrix from the local East-North-Up to Unreal at the
  * specified Unreal relative world location (relative to the floating
  * origin). The returned transformation works in Unreal's left-handed
  * coordinate system.
  */
  static glm::dmat3 ComputeEastNorthUpToUnreal(const glm::dvec3& Ue);
  
  /**
  * Computes the rotation matrix from the local East-North-Up to
  * Earth-Centered, Earth-Fixed (ECEF) at the specified ECEF location.
  */
  static glm::dmat3 ComputeEastNorthUpToEcef(const glm::dvec3& Ecef);


};
