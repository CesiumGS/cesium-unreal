# Change Log

### v1.21.0 - 2023-01-02

##### Fixes :wrench:

- Fixed a bug where Cesium for Unreal depended on a number of Unreal modules _privately_, but then used them from public headers. These are now declared as public dependencies. This could lead to compile errors in previous versions when attempting to include Cesium for Unreal headers from outside the project without also explicitly declaring `UMG` and other modules as dependencies.
- Fixed a bug that caused newly-created sub-levels to have their longitude and latitude parameters flipped relative to the current location of the `CesiumGeoreference`.

### v1.20.1 - 2022-12-09

##### Additions :tada:

- Added the ability to specify the endpoint URL of the Cesium ion API on a `CesiumIonRasterOverlay`.

##### Fixes :wrench:

- Fixed a bug that could cause crashes, including on startup, on non-Windows platforms.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.21.1 to v0.21.2. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.20.0 - 2022-12-02

##### Breaking Changes :mega:

- This is the _last release_ that will support Unreal Engine v4.26. Starting in the next release, in January 2023, UE 4.26 will no longer be supported. You may continue to use old versions of Cesium for Unreal in UE 4.26, but we recommend upgrading your UE version as soon as possible in order to continue receiving the latest updates.

##### Additions :tada:

- Added support for Unreal Engine v5.1.

##### Fixes :wrench:

- Fixed a bug that caused Cesium3DTilesets to fail to disconnect from CesiumGeoreference notifications. It could cause problems when changing to a different georeference instance.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.21.0 to v0.21.1. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.19.0 - 2022-11-01

##### Breaking Changes :mega:

- Removed some poorly named and unreliable functions on the `CesiumGeoreference`: `ComputeEastNorthUp`, `TransformRotatorEastNorthUpToUnreal`, and `TransformRotatorUnrealToEastNorthUp`. These functions have been replaced with reliable "EastSouthUp" counterparts.

##### Additions :tada:

- Added asynchronous texture creation where supported by the graphics API. This offloads a frequent render thread bottleneck to background loading threads.
- Improved synchronous texture creation by eliminating a main-thread memcpy, for cases where asynchronous texture creation is not supported.
- Added throttling of the main-thread part of loading for glTFs.
- Added throttling for tile cache unloads on the main thread.
- Added a prototype developer feature enabling Unreal Insights tracing into Cesium Native. This helps us investigate end-to-end performance in a deeper and more precise manner.

##### Fixes :wrench:

- Significantly reduced frame-rate dips during asynchronous tile loading by eliminating thread pool monopolization by Cesium tasks.
- Improved the tile destruction sequence by allowing it to defer being destroyed to future frames if it is waiting on asynchronous work to finish. Previously we would essentially block the main thread waiting for tiles to become ready for destruction.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.20.0 to v0.21.0. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.18.0 - 2022-10-03

##### Additions :tada:

- Improved the dithered transition between levels-of-detail, making it faster and eliminating depth fighting.
- Added an option to `Cesium3DTileset` to change the tileset's mobility, rather than always using Static mobility. This allows users to make a tileset movable at runtime, if needed.
- `ACesiumCreditSystem` now has a Blueprint-accessible property for the `CreditsWidget`. This is useful to, for example, move the credits to an in-game billboard rather than a 2D overlay.

##### Fixes :wrench:

- Fixed a bug where collision settings were only applied to the first primitive in a glTF.
- Fixed a bug where the Screen Credits Decorator prevented the Rich Text Block Image Decorator from working.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.19.0 to v0.20.0. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.17.0 - 2022-09-01

##### Additions :tada:

- The translucent parts of 3D Tiles are now correctly rendered as translucent. In addition, a new `TranslucentMaterial` property on `Cesium3DTileset` allows a custom material to be used to render these translucent portions.
- Added a `Rendering -> Use Lod Transitions` option on `Cesium3DTileset` to smoothly dither between levels-of-detail rather than switching abruptly.
- Added support for loading WebP images inside glTFs and raster overlays. WebP textures can be provided directly in a glTF texture or in the `EXT_texture_webp` extension.

##### Fixes :wrench:

- Fixed a bug that prevented fractional DPI scaling from being properly taken into account. Instead, it would scale by the next-smallest integer.
- Cesium for Unreal now only uses Editor viewports for tile selection if they are visible, real-time, and use a perspective projection. Previously, any viewport with a valid size was used, which could lead to tiles being loaded and rendered unnecessarily.
- Fixed a bug that caused tiles to disappear when the Editor viewport was in Orbit mode.
- Fixed a bug in the Globe Anchor Component that prevented changing/resetting the actor transform in the details panel.
- Reduced the size of physics meshes by only copying UV data if "Support UV from Hit Results" is enabled in the project settings.
- Fixed a bug - in Unreal Engine 5 only - where a LineTrace would occasionally fail to collide with tiles at certain levels-of-detail.
- Fixed a crash that could occur while running on a dedicated server, caused by attempting to add the credits widget to the viewport.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.18.1 to v0.19.0. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.16.2 - 2022-08-04

##### Fixes :wrench:

- Fixed a bug that caused a crash in Unreal Engine 4.26 when enabling the experimental tileset occlusion culling feature.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.18.0 to v0.18.1. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.16.1 - 2022-08-01

##### Fixes :wrench:

- Fixed a bug that could cause a crash when using thumbnail rendering, notably on the Project Settings panel in UE5.
- More fully disabled the occlusion culling system when the project-level feature flag is disabled.

### v1.16.0 - 2022-08-01

##### Breaking Changes :mega:

- Cesium for Unreal now automatically scales the selected 3D Tiles level-of-detail by the viewport client's `GetDPIScale`, meaning that devices with high-DPI displays will get less detail and higher performance than they did in previous versions. This can be disabled - and the previous behavior restored - by disabling the "Scale Level Of Detail By DPI" in Project Settings under Plugins -> Cesium, or by changing the "Apply Dpi Scaling" property on individual Tileset Actors.

##### Additions :tada:

- Added an experimental feature that uses Occlusion Culling to avoid refining tiles that are occluded by other content in the level. Currently this must be explicitly enabled from the Plugins -> Cesium section of Project Settings. We expect to enable it by default in a future release.
- Added options in `ACesium3DTileset` to control occlusion culling and turn it off if necessary.
- `UCesiumGltfPrimitiveComponent` now has static mobility, allowing it to take advantage of several rendering optimizations only available for static objects.
- Added an `OnTilesetLoaded` even that is invoked when the current tileset has finished loading. It is available from C++ and Blueprints.
- Added a `GetLoadProgress` method that returns the current load percentage of the tileset. It is available from C++ and Blueprints.
- Added Blueprint-accessible callback `OnFlightComplete` for when Dynamic Pawn completes flight.
- Added Blueprint-accessible callback `OnFlightInterrupt` for when Dynamic Pawn's flying is interrupted.

##### Fixes :wrench:

- When the Cesium ion access or login token is modified and stored in a config file that is under source code control, it will now be checked out before it is saved. Previously, token changes could be silently ignores with a source code control system that marks files read-only, such as Perforce.
- Fixed a bug that prevented credit images from appearing in UE5.
- Fixed a credit-related crash that occur when switching levels.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.17.0 to v0.18.0. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.15.0 - 2022-07-01

##### Additions :tada:

- Display credits using Rich Text Block instead of the Web Browser Widget.

##### Fixes :wrench:

- Swapped latitude and longitude parameters on georeferenced sublevels to match with the main georeference.
- Adjusted the presentation of sublevels in the Cesium Georeference details panel.
- We now explicitly free physics mesh UVs and face remap data, reducing memory usage in the Editor and reducing pressure on the garbage collector in-game.
- Fixed a bug that could cause a crash when reporting tileset or raster overlay load errors, particularly while switching levels.
- We now Log the correct asset source when loading a new tileset from either URL or Ion.

### v1.14.0 - 2022-06-01

##### Breaking Changes :mega:

- Renamed `ExcludeTilesInside` to `ExcludeSelectedTiles` on the `CesiumPolygonRasterOverlay`. A core redirect was added to remap the property value in existing projects.

##### Additions :tada:

- Added the `InvertSelection` option on `CesiumPolygonRasterOverlay` to rasterize outside the selection instead of inside. When used in conjunction with the `ExcludeSelectedTiles` option, tiles completely outside the polygon selection will be culled, instead of the tiles inside.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.15.1 to v0.16.0, fixing an important bug. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.13.2 - 2022-05-13

##### Fixes :wrench:

- Fixed a bug that could cause a crash after applying a non-UMaterialInstanceDynamic material to a tileset.
- Fixed a bug introduced in v1.13.0 that could lead to incorrect axis-aligned bounding boxes.
- Gave initial values to some fields in UStructs that did not have them, including two `UObject` pointers.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.15.1 to v0.15.2, fixing an important bug and updating some third-party libraries. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.13.1 - 2022-05-05

##### Breaking Changes :mega:

- Removed the following material assets that were accidentally included in the plugin in v1.13.0: `MetadataStyling/Layers/NYCBuildings_ByHeight_ML`, `MetadataStyling/Layers/NYCBuildings_ByYear_ML`, `MetadataStyling/NYCBuildings_ByHeight_M`, and `MetadataStyling/NYCBuildings_ByYear_M`. These assets can still be found in the [Cesium for Unreal Samples](https://github.com/CesiumGS/cesium-unreal-samples) project.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.15.0 to v0.15.1, fixing an important bug. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.13.0 - 2022-05-02

##### Breaking Changes :mega:

- Deprecated parts of the old Blueprint API for feature ID attributes from `EXT_feature_metadata`.

##### Additions :tada:

- Improved the Blueprint API for feature ID attributes from `EXT_feature_metadata` (and upgraded batch tables).
- Added a Blueprint API to access feature ID textures and feature textures from the `EXT_feature_metadata` extension.
- Added the `UCesiumEncodedMetadataComponent` to enable styling with the metadata from the `EXT_feature_metadata` extension. This component provides a convenient way to query for existing metadata, dictate which metadata properties to encode for styling, and generate a starter material layer to access the wanted properties.

##### Fixes :wrench:

- glTF normal, occlusion, and metallic/roughness textures are no longer treated as sRGB.
- Improved the computation of axis-aligned bounding boxes for Unreal Engine, producing much smaller and more accurate bounding boxes in many cases.
- Metadata-related Blueprint functions will now return the default value if asked for an out-of-range feature or array element. Previously, they would assert or read undefined memory.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.14.0 to v0.15.0. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.12.0 - 2022-04-01

##### Additions :tada:

- Raster overlays are now, by default, rendered using the default settings for the `World` texture group, which yields much higher quality on many platforms by enabling anisotrpic texture filtering. Shimmering of overlay textures in the distance should be drastically reduced.
- New options on `RasterOverlay` give the user control over the texture group, texture filtering, and mipmapping used for overlay textures.
- Improved the mapping between glTF textures and Unreal Engine texture options, which should improve texture quality in tilesets.
- Added `CesiumWebMapServiceRasterOverlay` to pull raster overlays from a WMS server.
- Added option to show `Cesium3DTileset` and `CesiumRasterOverlay` credits on screen, rather than in a separate popup.

##### Fixes :wrench:

- Fixed a leak of some of the memory used by tiles that were loaded but then ended up not being used because the camera had moved.
- Fixed a leak of glTF emissive textures.
- Fixed a bug introduced in v1.11.0 that used the Y-size of the right eye viewport for the left eye in tile selection for stereographic rendering.
- Fixed a bug where glTF primitives with no render data are added to the glTF render result.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.13.0 to v0.14.0. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.11.0 - 2022-03-01

##### Breaking Changes :mega:

- Exclusion Zones have been deprecated and will be removed in a future release. Please use the Cartographic Polygon Actor instead.

##### Additions :tada:

- Added experimental support for Unreal Engine 5 (preview 1).
- Added collision meshes for tilesets when using the Chaos physics engine.
- Integrated GPU pixel compression formats received from Cesium Native into Unreal's texture system.
- Added support for the `CESIUM_RTC` glTF extension.
- Added the ability to set tne Georeference origin from ECEF coordinates in Blueprints and C++.
- Exposed the Cesium ion endpoint URL as a property on tilesets and raster overlays.

##### Fixes :wrench:

- Fixed bug where certain pitch values in "Innaccurate Fly to Location Longitude Latitude Height" cause gimbal lock.
- Fixed a bug that caused a graphical glitch by using 16-bit indices when 32-bit indices are needed.
- Fixed a bug where tileset metadata from a feature table was not decoded correctly from UTF-8.
- Improved the shadows, making shadows fade in and out less noticable.
- The Cesium ion Token Troubleshooting panel will no longer appear in game worlds, including Play-In-Editor.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.12.0 to v0.13.0. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.10.1 - 2022-02-01

##### Fixes :wrench:

- Fixed a crash at startup on Android devices introduced in v1.10.0.

### v1.10.0 - 2022-02-01

##### Breaking Changes :mega:

- The following Blueprints and C++ functions on `CesiumSunSky` have been renamed. CoreRedirects have been provided to handle the renames automatically for Blueprints.
  - `EnableMobileRendering` to `UseMobileRendering`
  - `AdjustAtmosphereRadius` to `UpdateAtmosphereRadius`

##### Additions :tada:

- Added Cesium Cartographic Polygon to the Cesium Quick Add panel.
- Improved the Cesium ion token management. Instead of automatically creating a Cesium ion token for each project, Cesium for Unreal now prompts you to select or create a token the first time one is needed.
- Added a Cesium ion Token Troubleshooting panel that appears when there is a problem connecting to Cesium ion tilesets and raster overlays.
- The new `FCesiumCamera` and `ACesiumCameraManager` can be used to register and update custom camera views into Cesium tilesets.

##### Fixes :wrench:

- Fixed a crash when editing the georeference detail panel while a sublevel is active.
- Improved the organization of `CesiumSunSky` parameters in the Details Panel.
- Improved the organization of `CesiumGlobeAnchorComponent` parameters in the Details Panel.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.11.0 to v0.12.0. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.9.0 - 2022-01-03

##### Fixes :wrench:

- Fixed a bug that could cause incorrect LOD and culling when viewing a camera in-editor and the camera's aspect ratio does not match the viewport window's aspect ratio.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.10.0 to v0.11.0. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.8.1 - 2021-12-02

In this release, the cesium-native binaries are built using Xcode 11.3 on macOS instead of Xcode 12. Other platforms are unchanged from v1.8.0.

### v1.8.0 - 2021-12-01

##### Additions :tada:

- `Cesium3DTileset` now has options for enabling custom depth and stencil buffer.
- Added `CesiumDebugColorizeTilesRasterOverlay` to visualize how a tileset is divided into tiles.
- Added `Log Selection Stats` debug option to the `Cesium3DTileset` Actor.
- Exposed raster overlay properties to Blueprints, so that overlays can be created and manipulated with Blueprints.

##### Fixes :wrench:

- Cesium for Unreal now does a much better job of releasing memory when the Unreal Engine garbage collector is not active, such as in the Editor.
- Fixed a bug that could cause an incorrect field-of-view angle to be used for tile selection in the Editor.
- Fixed a bug that caused `GlobeAwareDefaultPawn` (and its derived classes, notably `DynamicPawn`) to completely ignore short flights.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.9.0 to v0.10.0. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.7.0 - 2021-11-01

##### Breaking Changes :mega:

- Removed `CesiumGlobeAnchorParent`, which was deprecated in v1.3.0. The `CesiumGlobeAnchorParent` functionality can be recreated using an empty actor with a `CesiumGlobeAnchorComponent`.
- Removed the `FixTransformOnOriginRebase` property from `CesiumGeoreferenceComponent`, and the component now always acts as if it is enabled. This should now work correctly even for objects that are moved by physics or other Unreal Engine mechanisms.
- The `SnapToEastSouthUp` function on `CesiumGeoreference` no longer resets the Scale back to 1.0. It only modifies the rotation.
- The following `CesiumGeoreferenceComponent` Blueprints and C++ functions no longer take a `MaintainRelativeOrientation` parameter. Instead, this behavior is controlled by the `AdjustOrientationForGlobeWhenMoving` property.
  - `MoveToLongitudeLatitudeHeight`
  - `InaccurateMoveToLongitudeLatitudeHeight`
  - `MoveToECEF`
  - `InaccurateMoveToECEF`
- Renamed `CesiumGeoreferenceComponent` to `CesiumGlobeAnchorComponent`.
- `CesiumSunSky` has been converted from Blueprints to C++. Backward compatibility should be preserved in most cases, but some less common scenarios may break.
- `GlobeAwareDefaultPawn`, `DynamicPawn`, and `CesiumSunSky` no longer have a `Georeference` property. Instead, they have a `CesiumGlobeAnchor` component that has a `Georeference` property.
- The `Georeference` property on most Cesium types can now be null if it has not been set explicitly in the Editor. To get the effective Georeference, including one that has been discovered in the level, use the `ResolvedGeoreference` property or call the `ResolveGeoreference` function.
- Removed the option to locate the Georeference at the "Bounding Volume Origin". It was confusing and almost never useful.
- The `CheckForNewSubLevels` and `JumpToCurrentLevel` functions in `CesiumGeoreference` have been removed. New sub-levels now automatically appear without an explicit check, and the current sub-level can be changed using the standard Unreal Engine Levels panel.
- Removed the `CurrentLevelIndex` property from `CesiumGeoreference`. The sub-level that is currently active in the Editor can be queried with the `GetCurrentLevel` function of the `World`.
- Removed the `SunSky` property from `CesiumGeoreference`. The `CesiumSunSky` now holds a reference to the `CesiumGeoreference`, rather than the other way around.
- The following Blueprints and C++ functions on `CesiumGeoreference` have been renamed. CoreRedirects have been provided to handle the renames automatically for Blueprints.
  - `TransformLongitudeLatitudeHeightToUe` to `TransformLongitudeLatitudeHeightToUnreal`
  - `InaccurateTransformLongitudeLatitudeHeightToUe` to `InaccurateTransformLongitudeLatitudeHeightToUnreal`
  - `TransformUeToLongitudeLatitudeHeight` to `TransformLongitudeLatitudeHeightToUnreal`
  - `InaccurateTransformUeToLongitudeLatitudeHeight` to `InaccurateTransformUnrealToLongitudeLatitudeHeight`
  - `TransformEcefToUe` to `TransformEcefToUnreal`
  - `InaccurateTransformEcefToUe` to `InaccurateTransformEcefToUnreal`
  - `TransformUeToEcef` to `TransformUnrealToEcef`
  - `InaccurateTransformUeToEcef` to `InaccurateTransformUnrealToEcef`
  - `TransformRotatorUeToEnu` to `TransformRotatorUnrealToEastNorthUp`
  - `InaccurateTransformRotatorUeToEnu` to `InaccurateTransformRotatorUnrealToEastNorthUp`
  - `TransformRotatorEnuToUe` to `TransformRotatorEastNorthUpToUnreal`
  - `InaccurateTransformRotatorEnuToUe` to `InaccurateTransformRotatorEastNorthUpToUnreal`
- The following C++ functions on `CesiumGeoreference` have been removed:
  - `GetGeoreferencedToEllipsoidCenteredTransform` and `GetEllipsoidCenteredToGeoreferencedTransform` moved to `GeoTransforms`, which is accessible via the `getGeoTransforms` function on `CesiumGeoreference`.
  - `GetUnrealWorldToEllipsoidCenteredTransform` has been replaced with `TransformUnrealToEcef` except that the latter takes standard Unreal world coordinates rather than absolute world coordinates. If you have absolute world coordinates, subtract the World's `OriginLocation` before calling the new function.
  - `GetEllipsoidCenteredToUnrealWorldTransform` has been replaced with `TransformEcefToUnreal` except that the latter returns standard Unreal world coordinates rather than absolute world coordinates. If you want absolute world coordinates, add the World's `OriginLocation` to the return value.
  - `AddGeoreferencedObject` should be replaced with a subscription to the new `OnGeoreferenceUpdated` event.

##### Additions :tada:

- Improved the workflow for managing georeferenced sub-levels.
- `CesiumSunSky` now automatically adjusts the atmosphere size based on the player Pawn's position to avoid tiled artifacts in the atmosphere when viewing the globe from far away.
- `GlobeAwareDefaultPawn` and derived classes like `DynamicPawn` now have a `CesiumGlobeAnchorComponent` attached to them. This allows more consistent movement on the globe, and allows the pawn's Longitude/Latitude/Height or ECEF coordinates to be specified directly in the Editor.
- `CesiumSunSky` now has an `EnableMobileRendering` flag that, when enabled, switches to a mobile-compatible atmosphere rendering technique.
- `CesiumCartographicPolygon`'s `GlobeAnchor` and `Polygon` are now exposed in the Editor and to Blueprints.
- Added `InaccurateGetLongitudeLatitudeHeight` and `InaccurateGetECEF` functions to `CesiumGlobeAnchorComponent`, allowing access to the current position of a globe-anchored Actor from Blueprints.
- Added support for collision object types on 'ACesium3DTileset' actors.

##### Fixes :wrench:

- Cesium objects in a sub-level will now successfully find and use the `CesiumGeoreference` and `CesiumCreditSystem` object in the Persistent Level when these properties are left unset. For best results, we suggest removing all instances of these objects from sub-levels.
- Fixed a bug that made the Time-of-Day widget forget the time when it was closed and re-opened.
- Undo/Redo now work more reliably for `CesiumGlobeAnchor` properties.
- We now explicitly free the `PlatformData` and renderer resources associated with `UTexture2D` instances created for raster overlays when the textures are no longer needed. By relying less on the Unreal Engine garbage collector, this helps keep memory usage lower. It also keeps memory from going up so quickly in the Editor, which by default does not run the garbage collector at all.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.8.0 to v0.9.0. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.6.3 - 2021-10-01

##### Fixes :wrench:

- Fixed a bug that caused incorrect tangents to be generated based on uninitialized texture coordinates.
- Fixed a bug that could cause vertices to be duplicated and tangents calculated even when not needed.
- Fixed a bug that caused the Cesium ion access token to sometimes be blank when adding an asset from the "Cesium ion Assets" panel while the "Cesium" panel is not open.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.7.2 to v0.8.0. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.6.2 - 2021-09-14

This release only updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.7.1 to v0.7.2. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.6.1 - 2021-09-14

##### Additions :tada:

- Added the `MaximumCachedBytes` property to `ACesium3DTileset`.

##### Fixes :wrench:

- Fixed incorrect behavior when two sublevels overlap each other. Now the closest sublevel is chosen in that case.
- Fixed crash when `GlobeAwareDefaultPawn::FlyToLocation` was called when the pawn was not possessed.
- Fixed a bug that caused clipping to work incorrectly for tiles that are partially water.
- Limited the length of names assigned to the ActorComponents created for 3D Tiles, to avoid a crash caused by an FName being too long with extremely long tileset URLs.
- Fixed a bug that caused 3D Tiles tile selection to take into account Editor viewports even when in Play-in-Editor mode.
- Fixed a bug in `DynamicPawn` that caused a divide-by-zero message to be printed to the Output Log.
- Fixed a mismatch on Windows between Unreal Engine's compiler options and cesium-native's compiler options that could sometimes lead to crashes and other broken behavior.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.7.0 to v0.7.1. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.6.0 - 2021-09-01

##### Breaking Changes :mega:

- Removed `ACesium3DTileset::OpacityMaskMaterial`. The regular `Material` property is used instead.
- Renamed `UCesiumMetadataFeatureTableBlueprintLibrary::GetPropertiesForFeatureID` to `UCesiumMetadataFeatureTableBlueprintLibrary::GetMetadataValuesForFeatureID`. This is a breaking change for C++ code but Blueprints should be unaffected because of a CoreRedirect.
- Renamed `UCesiumMetadataFeatureTableBlueprintLibrary::GetPropertiesAsStringsForFeatureID` to `UCesiumMetadataFeatureTableBlueprintLibrary::GetMetadataValuesAsStringForFeatureID`. This is a breaking change for C++ code but it was not previously exposed to Blueprints.

##### Additions :tada:

- Added the ability to define a "Cesium Cartographic Polygon" and then use it to clip away part of a Cesium 3D Tileset.
- Multiple raster overlays per tileset are now supported.
- The default materials used to render Cesium 3D Tilesets are now built around Material Layers, making them easier to compose and customize.
- Added support for using `ASceneCapture2D` with `ACesium3DTileset` actors.
- Added an editor option in `ACesium3DTileset` to optionally generate smooth normals for glTFs that originally did not have normals.
- Added an editor option in `ACesium3DTileset` to disable the creation of physics meshes for its tiles.
- Added a Refresh button on the Cesium ion Assets panel.
- Made `UCesiumMetadataFeatureTableBlueprintLibrary::GetMetadataValuesAsStringForFeatureID`, `UCesiumMetadataFeatureTableBlueprintLibrary::GetProperties`, and `UCesiumMetadataPrimitiveBlueprintLibrary::GetFirstVertexIDFromFaceID` callable from Blueprints.
- Consolidated texture preparation code. Now raster overlay textures can generate mip-maps and the overlay texture preparation can happen partially on the load thread.
- The Cesium ion Assets panel now has two buttons for imagery assets, allowing the user to select whether the asset should replace the base overlay or be added on top.

##### Fixes :wrench:

- Fixed indexed vertices being duplicated unnecessarily in certain situations in `UCesiumGltfComponent`.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.6.0 to v0.7.0. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.5.2 - 2021-08-30

##### Additions :tada:

- Added support for Unreal Engine v4.27.

### v1.5.1 - 2021-08-09

##### Breaking :mega:

- Changed Cesium Native Cesium3DTiles's namespace to Cesium3DTilesSelection's namespace

##### Fixes :wrench:

- Fixed a bug that could cause mis-registration of feature metadata to the wrong features in Draco-compressed meshes.
- Fixed a bug that could cause a crash with VR/AR devices enabled but not in use.

### v1.5.0 - 2021-08-02

##### Additions :tada:

- Added support for reading per-feature metadata from glTFs with the `EXT_feature_metadata` extension or from 3D Tiles with a B3DM batch table and accessing it from Blueprints.
- Added support for using multiple view frustums in `ACesium3DTileset` to inform the tile selection algorithm.

##### Fixes :wrench:

- Fixed a bug introduced in v1.4.0 that made it impossible to add a "Blank 3D Tiles Tileset" using the Cesium panel without first signing in to Cesium ion.
- Fixed a bug that caused a crash when deleting a Cesium 3D Tileset Actor and then undoing that deletion.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.5.0 to v0.6.0. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.4.1 - 2021-07-13

##### Fixes :wrench:

- Fixed linker warnings on macOS related to "different visibility settings."
- Fixed compile errors on Android in Unreal Engine versions prior to 4.26.2 caused by missing support for C++17.

### v1.4.0 - 2021-07-01

##### Breaking :mega:

- Tangents are now only generated for models that don't have them and that do have a normal map, saving a significant amount of time. If you have a custom material that requires the tangents, or need them for any other reason, you may set the `AlwaysIncludeTangents` property on `Cesium3DTileset` to force them to be generated like they were in previous versions.

##### Additions :tada:

- The main Cesium panel now has buttons to easily add a `CesiumSunSky` or a `DynamicPawn`.

##### Fixes :wrench:

- Fixed a bug that could sometimes cause tile-sized holes to appear in a 3D Tiles model for one render frame.
- Fixed a bug that caused Cesium toolbar buttons to disappear when `Editor Preferences` -> `Use Small Tool Bar Icons` is enabled.
- Added support for other types of glTF index accessors: `BYTE`, `UNSIGNED_BYTE`, `SHORT`, and `UNSIGNED_SHORT`.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.4.0 to v0.5.0. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.3.1 - 2021-06-02

- Temporarily removed support for the Android platform because it is causing problems in Epic's build environment, and is not quite production ready in any case.

### v1.3.0 - 2021-06-01

##### Breaking :mega:

- Tileset properties that require a tileset reload (URL, Source, IonAssetID, IonAccessToken, Materials) have been moved to `private`. Setter and getter methods are now provided for modifying them in Blueprints and C++.
- Deprecated `CesiumGlobeAnchorParent` and `FloatingPawn`. The `CesiumGlobeAnchorParent` functionality can be recreated using an empty actor with a `CesiumGeoreferenceComponent`. The `FloatingPawn` is now replaced by the `DynamicPawn`. In a future release, the `DynamicPawn` will be renamed to `CesiumFloatingPawn`.

##### Additions :tada:

- Added support for the Android platform.
- Added support for displaying a water effect for the parts of quantized-mesh terrain tiles that are known to be water.
- Improved property change checks in `Cesium3DTileset::LoadTileset`.
- Made origin rebasing boolean properties in `CesiumGeoreference` and `CesiumGeoreferenceComponent` blueprint editable.
- Made 3D Tiles properties editable in C++ and blueprints via getter/setter functions. The tileset now reloads at runtime when these properties are changed.
- Improvements to dynamic camera, created altitude curves for FlyTo behavior.
- Constrained the values for `UPROPERTY` user inputs to be in valid ranges.
- Added `M_CesiumOverlayWater` and `M_CesiumOverlayComplexWater` materials for use with water tiles.
- Exposed all tileset materials to allow for changes in editor.
- Added `TeleportWhenUpdatingTransform` boolean property to CesiumGeoreferenceComponent.
- Added a "Year" property to `CesiumSunSky`.
- Added the ability to use an external Directional Light with `CesiumSunSky`, rather than the embedded DirectionalLight component.

##### Fixes :wrench:

- Fixed a bug that caused rendering and navigation problems when zooming too far away from the globe when origin rebasing is enabled.
- Fixed a bug that caused glTF node `translation`, `rotation`, and `scale` properties to be ignored even if the node had no `matrix`.
- Cleaned up, standardized, and commented material and material functions.
- Moved all materials and material functions to the `Materials` subfolder.
- Set CesiumSunSky's directional light intensity to a more physically accurate value.
- Moved Latitude before Longitude on the `CesiumGeoreference` and `CesiumGeoreferenceComponent` Details panels.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.3.1 to v0.4.0. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.2.1 - 2021-05-13

##### Fixes :wrench:

- Fixed a regression in Cesium for Unreal v1.2.0 where `GlobeAwareDefaultPawn` lost its georeference during playmode.
- Fixed a regression in Cesium for Unreal v1.2.0 where the matrices in `CesiumGeoreference` were being initialized to zero instead of identity.
- Fixed a regression in Cesium for Unreal v1.2.0 that broke the ability to paint foliage on terrain and other tilesets.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.3.0 to v0.3.1. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.2.0 - 2021-05-03

##### Additions :tada:

- Added a dynamic camera that adapts to height above terrain.
- Added Linux support.
- Added support for Tile Map Service (TMS) raster overlays.

##### Fixes :wrench:

- Fixed issue where displayed longitude-latitude-height in `CesiumGeoreferenceComponent` wasn't updating in certain cases.
- `FEditorDelegates::OnFocusViewportOnActors` is no longer unnecessarily subscribed to multiple times.
- `Loading tileset ...` is now only written to the output log when the tileset actually needs to be reloaded.
- Fixed a bug where collision does not update correctly when changing properties of a tileset in the editor.
- Fixed a bug that caused tiles to disappear when "Suspend Update" was enabled.

### v1.1.1 - 2021-04-23

##### Fixes :wrench:

- Fixed a bug that caused tilesets added with the "Add Blank" button to cause an error during Play-In-Editor.
- Fixed a bug that caused `ACesiumGeoreference::TransformEcefToUe` to be much less precise than expected.
- Moved the `BodyInstance` property on `Cesium3DTileset` to the `Collision` category so that it can be modified in the Editor.

### v1.1.0 - 2021-04-19

##### Additions :tada:

- Added macOS support.
- Added support for the legacy `gltfUpAxis` property in a tileset `asset` dictionary. Although this property is **not** part of the specification, there are many existing assets that use this property and had been shown with a wrong rotation otherwise.
- Changed the log level for the tile selection output from `Display` to `Verbose`. With default settings, the output will no longer be displayed in the console, but only written to the log file.
- Added more diagnostic details to error messages for invalid glTF inputs.
- Added diagnostic details to error messages for failed OAuth2 authorization with `CesiumIonClient::Connection`.
- Added a `BodyInstance` property to `Cesium3DTileset` so that collision profiles can be configured.
- Added an experimental "Exclusion Zones" property to `Cesium3DTileset`. While likely to change in the future, it already provides a way to exclude parts of a 3D Tiles tileset to make room for another.

##### Fixes :wrench:

- Gave glTFs created from quantized-mesh terrain tiles a more sensible material with a `metallicFactor` of 0.0 and a `roughnessFactor` of 1.0. Previously the default glTF material was used, which has a `metallicFactor` of 1.0, leading to an undesirable appearance.
- Reported zero-length images as non-errors, because `BingMapsRasterOverlay` purposely requests that the Bing servers return a zero-length image for non-existent tiles.
- 3D Tiles geometric error is now scaled by the tile's transform.
- Fixed a bug that that caused a 3D Tiles tile to fail to refine when any of its children had an unsupported type of content.
- The `Material` property of `ACesium3DTiles` is now a `UMaterialInterface` instead of a `UMaterial`, allowing more flexibility in the types of materials that can be used.
- Fixed a possible crash when a `Cesium3DTileset` does not have a `CesiumGeoreference` or it is not valid.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.1.0 to v0.2.0. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.0.0 - 2021-03-30 - Initial Release

##### Features :tada:

- High-accuracy, global-scale WGS84 globe for visualization of real-world 3D content
- 3D Tiles runtime engine to stream massive 3D geospatial datasets, such as terrain, imagery, 3D cities, and photogrammetry
  - Streaming from the cloud, a private network, or the local machine.
  - Level-of-detail selection
  - Caching
  - Multithreaded loading
  - Batched 3D Model (B3DM) content, including the B3DM content inside Composite (CMPT) tiles
  - `quantized-mesh` terrain loading and rendering
  - Bing Maps and Tile Map Service (TMS) raster overlays draped on terrain
- Integrated with Cesium ion for instant access to cloud based global 3D content.
- Integrated with Unreal Engine Editor, Actors and Components, Blueprints, Landscaping and Foliage, Sublevels, and Sequencer.
