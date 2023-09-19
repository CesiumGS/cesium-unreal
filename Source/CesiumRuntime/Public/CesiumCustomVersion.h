// Copyright 2020-2021 CesiumGS, Inc. and Contributors
#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"

struct CESIUMRUNTIME_API FCesiumCustomVersion {
  enum Versions {
    // The version before any custom version was added to Cesium for Unreal
    BeforeCustomVersionWasAdded = 0,

    // Cesium3DTileset gained the TilesetSource property. In previous versions,
    // the tileset source was assumed to be the URL if one was supplied, and
    // Cesium ion otherwise.
    TilesetExplicitSource = 1,

    // The Georeferencing system was refactored.
    GeoreferenceRefactoring = 2,

    // The explicit Mobility property on Cesium3DTileset was removed, in favor
    // of the normal Mobility property on the RootComponent.
    TilesetMobilityRemoved = 3,

    VersionPlusOne,
    LatestVersion = VersionPlusOne - 1
  };

  // The GUID for the Cesium for Unreal plugin's custom version
  static const FGuid GUID;
};
