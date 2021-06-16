// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OriginPlacement.generated.h"

/**
 * An enumeration of the possible strategies for placing the origin of
 * a Georeference.
 */
UENUM(BlueprintType)
enum class EOriginPlacement : uint8 {
  /**
   * Use the tileset's true origin as the Actor's origin. For georeferenced
   * tilesets, this usually means the Actor's origin will be at the center
   * of the Earth.
   */
  TrueOrigin UMETA(DisplayName = "True origin"),

  /**
   * Use a custom position within the tileset as the Actor's origin. The
   * position is expressed as a longitude, latitude, and height, and that
   * position within the tileset will be at coordinate (0,0,0) in the Actor's
   * coordinate system.
   */
  CartographicOrigin UMETA(DisplayName = "Longitude / latitude / height")
};
