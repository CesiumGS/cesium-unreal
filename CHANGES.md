# Change Log

### v1.3.0 - ???

##### Additions  :tada:

- Added support for displaying a water effect for the parts of quantized-mesh terrain tiles that are known to be water.
- Improved property change checks in `Cesium3DTileset::LoadTileset`.
- Make origin rebasing boolean properties in `CesiumGeoreference` and `CesiumGeoreferenceComponent` blueprint editable.
- Improvements to dynamic camera, created altitude curves for FlyTo behavior.
- Constrained the values for `UPROPERTY` user inputs to be in valid ranges.

##### Fixes :wrench:

- Fixed a bug that caused glTF node `translation`, `rotation`, and `scale` properties to be ignored even if the node had no `matrix`.

### v1.2.1 - 2021-05-13

##### Fixes :wrench:

- Fixed a regression in Cesium for Unreal v1.2.0 where `GlobeAwareDefaultPawn` lost its georeference during playmode.
- Fixed a regression in Cesium for Unreal v1.2.0 where the matrices in `CesiumGeoreference` were being initialized to zero instead of identity.
- Fixed a regression in Cesium for Unreal v1.2.0 that broke the ability to paint foliage on terrain and other tilesets.

In addition to the above, this release updates [cesium-native](https://github.com/CesiumGS/cesium-native) from v0.3.0 to v0.3.1. See the [changelog](https://github.com/CesiumGS/cesium-native/blob/main/CHANGES.md) for a complete list of changes in cesium-native.

### v1.2.0 - 2021-05-03

##### Additions  :tada:

- Added a dynamic camera that adapts to height above terrain.
- Added Linux support.
- Added support for Tile Map Service (TMS) raster overlays.

##### Fixes :wrench:

* Fixed issue where displayed longitude-latitude-height in `CesiumGeoreferenceComponent` wasn't updating in certain cases.
* `FEditorDelegates::OnFocusViewportOnActors` is no longer unnecessarily subscribed to multiple times.
* `Loading tileset ...` is now only written to the output log when the tileset actually needs to be reloaded.
* Fixed a bug where collision does not update correctly when changing properties of a tileset in the editor.
* Fixed a bug that caused tiles to disappear when "Suspend Update" was enabled.

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

##### Features  :tada:

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
