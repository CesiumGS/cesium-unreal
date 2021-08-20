# Change Log

### Next Release - ?

##### Additions :tada:

- Added support for using `ASceneCapture2D` with `ACesium3DTileset` actors.

##### Fixes :wrench:

- Added missing `UFUNCTION` annotation for `UCesiumMetadataFeatureTableBlueprintLibrary::GetPropertiesAsStringsForFeatureID()`, `UCesiumMetadataFeatureTableBlueprintLibrary::GetProperties()`, and `UCesiumMetadataPrimitiveBlueprintLibrary::GetFirstVertexIDFromFaceID()`

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
