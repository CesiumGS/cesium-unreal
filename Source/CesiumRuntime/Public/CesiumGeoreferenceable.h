// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Cesium3DTilesSelection/BoundingVolume.h"
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include <optional>
#include "CesiumGeoreferenceable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UCesiumGeoreferenceable : public UInterface {
  GENERATED_BODY()
};

/**
 *
 */
class CESIUMRUNTIME_API ICesiumGeoreferenceable {
  GENERATED_BODY()

public:
  /**
   * Determines if the bounding volume of this georeferenceable object is
   * currently available.
   */
  virtual bool IsBoundingVolumeReady() const = 0;

  /**
   * Gets the bounding volume of this georeferenceable object. If the bounding
   * volume is not yet ready or if the object has no bounding volume, nullopt
   * will be returned. Use
   * {@see IsBoundingVolumeReady} to determine which of these is the case.
   */
  virtual std::optional<Cesium3DTilesSelection::BoundingVolume>
  GetBoundingVolume() const = 0;

  /**
   * Notifies georeferenced objects that the underlying georeference transforms
   * have been updated. The new transforms can be fetched from the
   * `CesiumWorldGeoreference` actor.
   */
  virtual void NotifyGeoreferenceUpdated() = 0;
};
