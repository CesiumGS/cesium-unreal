// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Cesium3DTiles/BoundingVolume.h"
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CesiumBoundingVolumeProvider.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UCesiumBoundingVolumeProvider : public UInterface {
  GENERATED_BODY()
};

/**
 *
 */
class CESIUMRUNTIME_API ICesiumBoundingVolumeProvider {
  GENERATED_BODY()

public:

  /**
   * Gets the bounding volume of this georeferenceable object. If the bounding
   * volume is not yet ready or if the object has no bounding volume, nullopt
   * will be returned. Use
   * {@see IsBoundingVolumeReady} to determine which of these is the case.
   */
  virtual std::optional<Cesium3DTiles::BoundingVolume> GetBoundingVolume() const = 0;
};
