# Cesium for Unreal v2.0 Upgrade Guide

As of v2.0.0, Cesium for Unreal supports the `EXT_mesh_features` and `EXT_structural_metadata` extensions from 3D Tiles 1.1. Models with `EXT_features_metadata` will still load, but their feature IDs and metadata can no longer be accessed. The significant differences between the extensions required a large overhaul of the metadata-accessing API. There are measures in-place to ensure backwards compatibility and replace old Blueprints with the new nodes, but be sure to make a backup of your project before switching Cesium for Unreal versions, just to be sure.

This guide intends to inform users of the differences between the old and new metadata APIs, as well as how to achieve the same functionality with Blueprints.

## Table of Contents

- [Retrieving feature IDs from `EXT_mesh_features`](#retrieving-feature-ids-from-ext-mesh-features)
- [Retrieving metadata from `EXT_structural_metadata`](#retrieving-metadata-from-ext-structural-metadata)

## Retrieving feature IDs from `EXT_mesh_features`

In `EXT_feature_metadata`, feature IDs and metadata are stored together in the extension. In 3D Tiles 1.1, feature IDs are indicated by the `EXT_mesh_features` extension, independent of metadata. Thankfully, the new extension does not result in many differences in the Cesium for Unreal API. The most drastic change is the deprecation of `FCesiumMetadataPrimitive` -- feature IDs must now be retrieved from `FCesiumPrimitiveFeatures`.

See [here](https://github.com/CesiumGS/glTF/tree/3d-tiles-next/extensions/2.0/Vendor/EXT_mesh_features) for the `EXT_mesh_features` specification.

### Summary

- `FCesiumMetadataPrimitive` has been deprecated. Use `FCesiumPrimitiveFeatures` to enact on feature IDs stored in the `EXT_mesh_features` extension of a glTF primitive.
- Added `FCesiumFeatureIdSet`, which represents a feature ID set in `EXT_mesh_features`. A `FCesiumFeatureIdSet` has a `ECesiumFeatureIdSetType` indicating whether it is a feature ID attribute, a feature ID texture, or a set of implicit feature IDs.
- Added `UCesiumFeatureIdSetBlueprintLibrary`, which acts on an input `FCesiumFeatureIdSet`.
- Added `FCesiumFeatureIdTexture.GetFeatureIDForVertex`, which can retrieve the feature ID of the given vertex if it contains texture coordinates.
- Added `ECesiumFeatureIdAttributeStatus` and `ECesiumFeatureIdTextureStatus` to indicate whether a feature ID attribute or texture is valid, respectively.
- `UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureTableName` and `UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureTableName` have been deprecated. Instead, use `UCesiumFeatureIdSetBlueprintLibrary::GetPropertyTableIndex` to retrieve the index of a property table.

### Feature ID Sets

Feature IDs are stored in a `FCesiumFeatureIdSet`. A `FCesiumFeatureIdSet` has a `ECesiumFeatureIdSetType` indicating whether it is a feature ID attribute, a feature ID texture, or a set of implicit feature IDs.

The feature ID of a given vertex can be obtained in Blueprints with the **"Get Feature ID For Vertex"** node (or in C++, `UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDForVertex`). This will sample a `FCesiumFeatureIdSet` for the feature ID, regardless of its type.

!["Get Feature ID For Vertex" node in Blueprints](Images/getFeatureIdForVertex.jpeg)

If the `FCesiumFeatureIdSet` is a feature ID attribute type, the **"Get As Feature ID Attribute"** node can be used to interact with the underlying `FCesiumFeatureIDAttribute`. Similarly, if the `FCesiumFeatureIdSet` is a feature ID texture type, the **"Get As Feature ID Texture"** can be used. In C++, these functions are `UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDAttribute` and `UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDTexture` respectively.

!["Get As Feature ID Attribute" and "Get As Feature ID Texture" nodes in Blueprints](Images/getAsFeatureIdFunctions.jpeg)

Implicit feature ID sets have no corresponding underlying implementations – they simply correspond to the indices of vertices in the mesh.

### Interfacing with Property Tables

Feature IDs are associated with feature tables by name in `EXT_feature_metadata`. This name was used to retrieve the corresponding feature table from a map of feature tables in the model's root extension.

This changes with 3D Tiles 1.1. In `EXT_mesh_features`, feature IDs are optionally associated with metadata property tables from `EXT_structural_metadata`. If a `FCesiumFeatureIDSet` is associated with a property table, it will have a property table index. This value indexes into an array of property tables in the model's root extension.

The property table index can be retrieved with the **"Get Property Table Index"** Blueprint node (or in C++,`UCesiumFeatureIdSetBlueprintLibrary::GetPropertyTableIndex`). See the Property Tables section for more information.

### Feature ID Attributes and Textures

`FCesiumFeatureIdAttribute` and `FCesiumFeatureIdTexture` are mostly unchanged. For reasons explained above, their **"GetFeatureTableName"** Blueprints functions have been deprecated (`UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureTableName` and `UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureTextureName` in C++).

Previously, Cesium for Unreal would not indicate if a feature ID attribute or texture was somehow broken. For example, if a feature ID texture's image could not be loaded, nothing in the API would indicate that. Thus, the `ECesiumFeatureIdAttributeStatus` and `ECesiumFeatureIdTextureStatus` enums were added, to indicate when something in the feature ID sets is invalid. These can be queried using the **"Get Feature ID Attribute Status"** and **"Get Feature ID Texture Status"** Blueprints nodes respectively (or in C++, `UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureIDAttributeStatus` and `UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus` respectively).

!["Get Feature ID Attribute Status" and "Get Feature ID Texture Status" nodes in Blueprints](Images/getFeatureIdStatusFunctions.jpeg)

This can be used for debugging and validation purposes, e.g., to check if a `FCesiumFeatureIdAttribute` or `FCesiumFeatureIdTexture` are valid before trying to sample them for feature IDs.

Furthermore, if the **"Get As Feature ID Attribute"** or **"Get As Feature ID Texture"** nodes are used on a `FCesiumFeatureIdSet` of the wrong type, they will return `FCesiumFeatureIdAttribute` and `FCesiumFeatureIdTexture` instances with invalid statuses.

### Primitive Features

The `FCesiumPrimitiveFeatures` struct acts as a Blueprints-accessible version of `EXT_mesh_features`. Notably, it allows you to access all of the feature ID sets of a primitive using the **"Get Feature ID Sets"** Blueprints function (`UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets`). You can also use  **"Get Feature ID Sets Of Type"** (`UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSetsOfType`) to filter for a specific type of feature IDs.

You can also use the **"Get Feature ID From Face"** function (`UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace`) to get the feature ID that is associated with a given face index, from the specified `FCesiumFeatureIdSet`. Here's an example of how one might retrieve the feature ID of a primitive hit by a `LineTrace`:

![Example feature ID picking script](Images/getFeatureIdFromFaceExample.jpeg)

**Note**: This function does not interface well with feature ID textures or implicit feature IDs, since these feature ID types make it possible for a face to have multiple feature IDs. In these cases, the feature ID of the first vertex of the face is returned.

## Retrieving metadata from `EXT_structural_metadata`

`EXT_structural_metadata` builds upon the class properties possible in `EXT_feature_metadata` by adding new types. This type system is much more expansive, and as such, required complete rework of the metadata type system in Cesium for Unreal.

### True Type -> Value Type

The `ECesiumMetadataTrueType` enum has been completely deprecated. Now, the more detailed `EXT_structural_metadata` types are indicated through the `FCesiumMetadataValueType`. A `FCesiumMetadataValueType` has a `ECesiumMetadataType`, a `ECesiumMetadataComponentType`, and a boolean indicating whether the type is an array. The component type enum is only applicable to numeric types, i.e., scalars, `vecNs`, or `matN`s. Some examples of how these types are used are as follows:

| `FCesiumMetadataValueType`` | Explanation |
| ------------------------ | ------------ |
| Type: `ECesiumMetadataType::Boolean`<br/>ComponentType: `ECesiumMetadataComponentType::None`<br/>bIsArray: `false` | Describes a boolean property. Values are retrieved as booleans. |
| Type: `ECesiumMetadataType::Vec2`<br/>ComponentType: `ECesiumMetadataComponentType::Uint8`<br/>bIsArray: `false` | Describes a `vec2` property where the vectors contain unsigned 8-bit integer components. Values are retrieved as two-dimensional unsigned 8-bit integer vectors. |
| Type: `ECesiumMetadataType::String`<br/>ComponentType: `ECesiumMetadataComponentType::None`<br/>bIsArray: `true` | Describes a string array property. Values are retrieved as arrays of strings. |
| Type: `ECesiumMetadataType::Scalar`<br/>ComponentType: `ECesiumMetadataComponentType::Float32`<br/>bIsArray: `true` | Describes a scalar array property where the scalars are single-precision floats. Values are retrieved as arrays of single-precision floats. |

### Expanded Blueprint Types

Many of the `EXT_structural_metadata` types cannot be directly represented by Unreal Blueprint types. The `ECesiumMetadataBlueprintType` is still used to indicate the best-fitting Blueprints type for a metadata property or value. In Cesium for Unreal v2.0.0, it has been expanded to include the vector and matrix types that are possible with the `EXT_structural_metadata` extension.

### Metadata Arrays and Values

Renamed `FCesiumMetadataGenericValue` to `FCesiumMetadataValue`.

### Feature Tables -> Property Tables

Feature tables in `EXT_feature_metadata` correspond to property tables in `EXT_structural_metadata`. As such, `FCesiumFeatureTable` has been renamed to `FCesiumPropertyTable`. `FCesiumPropertyTableStatus` has been added to indicate whether a property table is valid.

Additionally, `UCesiumFeatureTableBlueprintLibrary` has been renamed to `UCesiumPropertyTableBlueprintLibrary`. The following functions have also been renamed:
| Old | New |
| --- | ----|
| `GetNumberOfFeatures` | `GetPropertyTableSize`|
| `GetMetadataValuesForFeatureID` | `GetMetadataValuesForFeature`|
| `GetMetadataValuesAsStringForFeatureID` | `GetMetadataValuesForFeatureAsStrings`|

Additionally, the `FCesiumMetadataProperty` struct, which represented a feature table property in `EXT_feature_metadata`, has been renamed to `FCesiumPropertyTableProperty`. 
  - Renamed `UCesiumMetadataPropertyBlueprintLibrary` to `UCesiumPropertyTablePropertyBlueprintLibrary`. `GetNumberOfFeatures` is now `GetPropertySize`,`GetComponentCount` is now `GetPropertyArraySize`, and `GetBlueprintComponentType` is now `GetArrayElementBlueprintType`.

  ### Feature Textures -> Property Textures
  - Renamed `FCesiumFeatureTexture` to `FCesiumPropertyTexture`.
  - Renamed `FCesiumFeatureTextureProperty` to `FCesiumPropertyTextureProperty`.
  - Renamed `UCesiumFeatureTexturePropertyBlueprintLibrary` to `UCesiumPropertyTexturePropertyBlueprintLibrary`. `GetPropertyKeys` is now `GetPropertyNames`.

- Added `FCesiumPropertyArray`, which represents an array retrieved from the `EXT_structural_metadata` extension.
- Renamed `FCesiumMetadataModel` to `FCesiumModelMetadata`, which represents the metadata specified by the `EXT_structural_metadata` extension on the root glTF model.
- Added `FCesiumPrimitiveMetadata`, which represents the metadata specified by the `EXT_structural_metadata` extension on a glTF mesh primitive.

- `FCesiumMetadataPrimitive` has been deprecated. Instead, use `FCesiumPrimitiveFeatures` to access the feature IDs of a primitive and `FCesiumPrimitiveMetadata` to access its metadata.
- `UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureTableName` and `UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureTableName` have been deprecated, since they are less applicable in `EXT_mesh_features`. Use `UCesiumFeatureIdSetBlueprintLibrary::GetPropertyTableIndex` instead.
- `UCesiumMetadataPrimitiveBlueprintLibrary::GetFirstVertexIDFromFaceID` has been deprecated. Use `UCesiumPrimitiveFeaturesBlueprintLibrary::GetFirstVertexFromFace` instead.
- `UCesiumMetadataUtilityBlueprintLibrary::GetFeatureIDFromFaceID` has been deprecated. Use `UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace` instead.
- `ECesiumMetadataTrueType` has been deprecated.
- `FCesiumMetadataArray` has been deprecated. Use `FCesiumPropertyArray` instead.