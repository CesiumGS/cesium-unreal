# Change Log

### ? - ?

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

##### Fixes :wrench:

- Cesium objects in a sub-level will now successfully find and use the `CesiumGeoreference` and `CesiumCreditSystem` object in the Persistent Level when these properties are left unset. For best results, we suggest removing all instances of these objects from sub-levels.
- Fixed a bug that made the Time-of-Day widget forget the time when it was closed and re-opened.
- Undo/Redo now work more reliably for `CesiumGlobeAnchor` properties.

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
