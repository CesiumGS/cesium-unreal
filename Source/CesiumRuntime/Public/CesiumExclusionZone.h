// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"

#include "CesiumExclusionZone.generated.h"

/**
 *
 */
USTRUCT()
struct FCesiumExclusionZone {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, Category = "Cesium")
  double South = 0.0;

  UPROPERTY(EditAnywhere, Category = "Cesium")
  double West = 0.0;

  UPROPERTY(EditAnywhere, Category = "Cesium")
  double North = 0.0;

  UPROPERTY(EditAnywhere, Category = "Cesium")
  double East = 0.0;
};
