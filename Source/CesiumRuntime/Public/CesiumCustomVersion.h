// Copyright 2020-2024 CesiumGS, Inc. and Contributors
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

    // The UCesiumGlobeAnchorComponent's globe transformation changed from being
    // an array of doubles to being an FMatrix.
    GlobeAnchorTransformationAsFMatrix = 4,

    // The origin shifting behavior became an independent component rather than
    // built into the CesiumGeoreference.
    OriginShiftComponent = 5,

    // Fly-to behavior became an independent component rather than built into
    // the GlobeAwareDefaultPawn.
    FlyToComponent = 6,

    // Added the CesiumIonServer property to Cesium3DTileset and
    // CesiumIonRasterOverlay.
    CesiumIonServer = 7,

    // Replaced the UseWebMercatorProjection property in
    // CesiumWebMapTileServiceOverlay with the enum Projection property.
    WebMapTileServiceProjectionAsEnum = 8,

    VersionPlusOne,
    LatestVersion = VersionPlusOne - 1
  };

  // The GUID for the Cesium for Unreal plugin's custom version
  static const FGuid GUID;
};
