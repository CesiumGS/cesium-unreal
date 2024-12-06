# Reference Frames {#reference-frames}

This is an inventory of the most important reference frames used in Cesium for Unreal.
<!--! [TOC] -->

## Ellipsoid-centered

This is the "native" reference frame of Cesium.

|               |                                                                                                                                                                                                                                                                                                         |
| ------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| _Handedness_  | Right                                                                                                                                                                                                                                                                                                   |
| _Units_       | Meters                                                                                                                                                                                                                                                                                                  |
| _Origin_      | Center of the ellipsoid (Earth)                                                                                                                                                                                                                                                                         |
| _Orientation_ | <ul><li>+X passes through the intersection of the equator and prime meridian (0 degrees latitude, 0 degrees longitude)</li><li>+Y passes through the intersection of the equator and +90 degrees longitude (0 degrees latitude, 90 degrees longitude)</li><li>+Z is up through the North Pole</li></ul> |

## Georeferenced

A reference frame defined by the `ACesiumGeoreference` actor.

|               |                                                                                                                                                                                                                                                                                                                                                                           |
| ------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| _Handedness_  | Right                                                                                                                                                                                                                                                                                                                                                                     |
| _Units_       | Meters                                                                                                                                                                                                                                                                                                                                                                    |
| _Origin_      | The origin defined by the `ACesiumGeoreference::OriginPlacement` and possibly `OriginLongitude`, `OriginLatitude`, and `OriginHeight` properties.                                                                                                                                                                                                                         |
| _Orientation_ | <ul><li>If `ACesiumGeoreference::AlignTilesetUpWithZ` is _true_: <ul><li>+X points East at the origin</li><li>+Y points North at the origin</li><li>+Z is in the direction of the ellipsoid surface normal at the origin (up)</li></ul> </li><li>If `ACesiumGeoreference::AlignTilesetUpWithZ` is _false_: <ul><li>Same as Ellipsoid-centered above.</li></ul> </li></ul> |

## Cesium Tileset

The reference frame of a Cesium 3D Tiles tileset, as defined by the tileset.json. Usually, tilesets are georeferenced and this reference frame is identical to the `Ellipsoid-centered` frame described above, but this is not strictly required by 3D Tiles. A non-georeferenced model of a building, for example, may have an origin at the center of the building and axes aligned with the principal sides of the building.

|               |                                                           |
| ------------- | --------------------------------------------------------- |
| _Handedness_  | Right                                                     |
| _Units_       | Meters                                                    |
| _Origin_      | Specified by tileset.json, often the center of the Earth. |
| _Orientation_ | Specified by tileset.json, often ECEF.                    |

## Unreal Tileset

The same as the Cesium Tileset, but expressed in Unreal Engine terms: the coordinates are in centimeters (multiplied by 100.0) and left-handed (have an inverted Y component) relative to Cesium Tileset coordinates.

Please note that the transformation from Unreal Tileset coordinates to Unreal Absolute/Relative World (below) is affected by the ACesium3DTileset Actor's Location and Orientation properties. However, these should almost always be set to identity.

|               |                                                           |
| ------------- | --------------------------------------------------------- |
| _Handedness_  | Left                                                      |
| _Units_       | Centimeters                                               |
| _Origin_      | Specified by tileset.json, often the center of the Earth. |
| _Orientation_ | Specified by tileset.json, often ECEF.                    |

## Unreal Absolute World

Vectors and matrices in Unreal Engine are expressed using single-precision floating-point numbers. In order to maintain precision, these coordinate values must remain relatively small. To support this, the Unreal floating world origin can be moved by setting the `OriginLocation` property of `UWorld`. Coordinates that are said to be in the "Unreal Absolute World" reference frame are expressed relative to the absolute origin (0,0,0) and are not affected by the value of the `OriginLocation` property.

|               |                   |
| ------------- | ----------------- |
| _Handedness_  | Left              |
| _Units_       | Centimeters       |
| _Origin_      | No fixed meaning. |
| _Orientation_ | No fixed meaning. |

## Unreal Relative World

This reference frame has the same orientation as "Unreal Absolute World", but is offset from it by the `UWorld`'s `OriginLocation`. Coordinates are expressed relative to the floating origin.

|               |                   |
| ------------- | ----------------- |
| _Handedness_  | Left              |
| _Units_       | Centimeters       |
| _Origin_      | No fixed meaning. |
| _Orientation_ | No fixed meaning. |
