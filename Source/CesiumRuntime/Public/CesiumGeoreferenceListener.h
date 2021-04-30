// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CesiumGeoreferenceListener.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UCesiumGeoreferenceListener : public UInterface {
  GENERATED_BODY()
};

/**
 *
 */
class CESIUMRUNTIME_API ICesiumGeoreferenceListener {
  GENERATED_BODY()

public:
  /**
   * Notifies georeferenced objects that the underlying georeference transforms
   * have been updated. The new transforms can be fetched from the
   * `CesiumWorldGeoreference` actor.
   */
  virtual void NotifyGeoreferenceUpdated() = 0;
};
