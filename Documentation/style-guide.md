# Cesium for Unreal Style Guide

Cesium strives to maintain a high standard for code quality across all of its codebases. The effort to write readable, organized, and consistent code makes it much easier for contributors to work with the code, whether new or experienced.

That said, the Cesium for Unreal plugin is a bit special. It combines code from [Cesium Native](https://github.com/CesiumGS/cesium-native/) and Unreal Engine, two large codebases with diverging guidelines, libraries, and design decisions. This guide attempts to reconcile between these two codebases and make recommendations where there are conflicts.

Before diving into this guide, we recommend the following:

1. Browse Cesium Native's [C++ style guide](https://github.com/CesiumGS/cesium-native/blob/main/doc/style-guide.md) to familiarize yourself with fundamental principles and expectations for C++ development.

2. Go through the [Unreal Engine Coding Standard](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine) for the official coding guidelines for Unreal Engine.

This guide will explicitly link to sections in the above resources when relevant, but it is good practice to learn the overall expectations of both environments.

**IN GENERAL**, if you're writing code for a public API, defer to Unreal Engine's style and code standards. When working with private APIs or implementation details, fall back on Cesium Native's C++ style. Use existing, similar code to infer the appropriate style for your own.

## Table of Contents

- [Naming](#naming)
- [Types](#types)
- [Documentation](#documentation)
- [File Organization](#file-organization)

## Naming

First, check out the [Naming Conventions](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine#namingconventions) section in the Unreal Engine coding standard. Then, read below.

### General

- You will have to follow Unreal's conventions for naming new structs or classes, e.g, prefixing structs with `F` or actor subclasses with `A`. If these prefixes are absent, the Unreal Header Tool will likely complain during the build process, and subsequently fail to compile your code.

- Although the names should be prefixed in code, the files containing them should *not* follow this rule. For example, `ACesium3DTileset` is defined in `Cesium3DTileset.h` and implemented in `Cesium3DTileset.cpp`, and *not* `ACesium3DTileset.h` or `ACesium3DTileset.cpp`.

- Every `struct` or `class` in the public API should be prefaced with the word "Cesium". This helps to clearly separate our API from other elements in Unreal, and allows users to more easily search for our components in Unreal.

```cpp
// Don't do this.
public AGeoreference : public AActor {...}

// Do this.
public ACesiumGeoreference : public AActor {...}
```

> Note that the above is not enforced for classes that are used in private implementations. For example, users won't ever interact directly with `LoadModelResult` or `UnrealAssetAccessor`, so these do not include a "Cesium" prefix.

### Public vs. Private Style

Functions and fields in the public API should be written in `PascalCase`. However, continue to use `lowerCamelCase` for any private members or anonymous namespace functions.

```cpp
USTRUCT()
struct FCesiumStruct {
    GENERATED_BODY()

public:
    float PublicField;
    void DoSomethingPublic(float Input);

private:
    float _privateField;
    void doSomethingPrivate(float input);
}
```

For pointer variables that are mentioned in the public API, e.g., public fields or function parameters, omit the `p` prefix. However, continue to use `p` to prefix pointers in private code.

```cpp
UCLASS()
class UCesiumClass {
    GENERATED_BODY()

public:
    UCesiumEllipsoid* Ellipsoid;
    void SetEllipsoid(UCesiumEllipsoid* NewEllipsoid)

private:
    ACesium3DTileset* _pTilesetActor;
    void setTileset(ACesium3DTileset* pNewTileset);
}
```

### Clarity

Aim to assign names to variables, functions, etc. that are succinctly descriptive. This is crucial for creating an intuitive user experience with the public API, but even private code should aspire to this standard.

**When in doubt, err on the side of being overly explicit.** Try to be precise about scope and meaning.

For example, consider the context of multiple frames of reference in Cesium for Unreal. The plugin has to operate in multiple coordinate systems: Unreal's coordinate system, Earth-Centered Earth-Fixed coordinates, and cartographic coordinates (longitude, latitude, height). Whenever something deals with position or orientation, it is important to distinguish what space it is operating in.

This intentionally vague example leaves a lot to be desired:

```cpp
FMatrix ComputeTransform(FVector Location) {...}
```

From a glance, it is not clear what the intended frame-of-reference is. (Maybe you assumed it was in Unreal space due to `Location`, but what is `Transform` for?) The function body might give us a hint, but the declaration could use more clarity. Adding documentation will definitely help here, and in some cases that is enough to avoid ambiguity.

```cpp
/**
 * Computes the matrix that transforms from an East-South-Up frame centered at
 * a given location to the Unreal frame.
 * 
 * [Detailed explanation of East-South-Up.]
 */
FMatrix ComputeTransform(FVector Location) {...}
```

However, we can take this even a step further. Let's rename the function and its parameter so that the intent is very obvious from the start.

```cpp
/**
 * Computes the matrix that transforms from an East-South-Up frame centered at
 * a given location to the Unreal frame.
 * 
 * [Detailed explanation of East-South-Up.]
 */
FMatrix ComputeEastSouthUpToUnrealTransformation(FVector UnrealLocation) {...}
```

In the same vein, avoid abbreviations unless they are commonly accepted or used across the codebase. This improves readability such that code is understandable to new developers from a glance. (Additionally, consider developers for whom English is not their first language; the arbitrary abbreviations can be even more confusing.)

```cpp
// Not great.
static void CompBoundingRect(FVector BL, FVector TR) {...}

// Better.
static void ComputeBoundingRectangle(FVector BottomLeft, FVector TopRight) {...}
```

We also write out "Earth-Centered, Earth-Fixed" and "Longitude, Latitude, Height" in the plugin. Though this makes for longer names, it is helpful for users who are new to geospatial and aren't familiar with abbreviations like ECEF. However, we still use WGS84 because the full name is cumbersome, and less helpful when spelled out.

### Units

Units should stay consistent across the API to avoid confusion as values are passed. In particular:

- Unreal uses centimeter units, while Cesium for Unreal uses meters. Meters are the standard units across our runtimes (like CesiumJS).
- Longitude and latitude are expressed in degrees, while height above the WGS84 ellipsoid is expressed in meters.

It's encouraged to leave documentation about the expected units and/or frame of reference for a function and its parameters. This comment in `CesiumGeoreference.h` is a good example:

```cpp
  /**
   * Transforms a position in Unreal coordinates into longitude in degrees (x),
   * latitude in degrees (y), and height above the ellipsoid in meters (z). The
   * position should generally not be an Unreal _world_ position, but rather a
   * position expressed in some parent Actor's reference frame as defined by its
   * transform. This way, the chain of Unreal transforms places and orients the
   * "globe" in the Unreal world.
   */
  FVector TransformUnrealPositionToLongitudeLatitudeHeight(
      const FVector& UnrealPosition) const;
```

## Types

### Pointers

Unreal provides engine-specific pointer types that may not be obvious to distinguish from one another. This [community guide](https://dev.epicgames.com/community/learning/tutorials/kx/unreal-engine-all-about-soft-and-weak-pointers) goes into detail about the differences between these types, but they are summarized for convenience below.

| Type | Description |
| ---- | ----------- |
| `TObjectPtr` | An optional replacement for raw pointers. The type `T` should be complete and derived from `UObject`. |
| `TSoftObjectPtr` | Used in UProperties.|
| `TWeakObjectPtr` | Used to reference already instantiated objects. Will resolve to null if the object gets destroyed or garbage collected.|

Generally, the above pointers are used whenever you instantiate something that inherits from an Unreal type (e.g., `UObject`, `USceneComponent`). Of course, raw pointers may be used where ownership is not required.

Unreal also provides `TUniquePtr` and `TSharedPtr` definitions, equivalent to their `std` counterparts. For consistency, Cesium for Unreal prefers using `TUniquePtr` and `TSharedPtr`. This is especially important when the type `T` is a `UObject`.

### Math

Unreal contains various vector and matrix classes that are functionally similar to the classes in `glm`, e.g., `FVector` for `glm::dvec3`, `FMatrix` for `glm::dmat4`.

However, if you ever find yourself doing any vector and matrix arithmetic, it is **strongly encouraged** to do operations with `glm` first. The computed result can later be copied into the corresponding Unreal Engine type.

This is especially recommended for matrix math. Previously, `FMatrix` made strong assumptions about the content of the matrix that could result in unexpected behaviors. It also defined operators in a way that deviates from typical linear algebra expectations. If you need the result to be an `FMatrix`, first use `glm::dmat4` for the computation, and *then* copy it into an `FMatrix`.

> Example of faulty `FMatrix` math:
>
> The `FMatrix::operator*` composed matrices in the opposite of the normal order. If you wanted a matrix that transforms by `A` then by `B`, Unreal would want you to multiply `A * B`. You would otherwise multiply `B * A` in `glm`.
>
> (This may no longer be true, but it has ultimately proven safer to use `glm::dmat4` over Unreal's built-in matrix.)

## Documentation

Documentation is generated by Doxygen before being published [online](https://cesium.com/learn/cesium-unreal/ref-doc). Therefore, any public API should be documented with Doxygen-compatible comments that follow the standards put forth by [Cesium Native](https://github.com/CesiumGS/cesium-native/blob/main/doc/topics/style-guide.md#-documentation).

The sole exception is to avoid the use of `@copydoc` in Cesium for Unreal. Unreal Engine uses comments to generate tooltips for object properties and Blueprints functions, but it doesn't parse every Doxygen command. So when the user hovers over such items, the tooltip often copies the text verbatim. While this isn't an issue for most comments,  `@copydoc` will never copy its intended comment, so the tooltip will be left as `"@copydoc"`. We circumvent this by manually duplicating the documentation comments as necessary.

## File Organization

Cesium for Unreal's source code (located in the `Source` folder) is divided into `Runtime` and `Editor`. The `Editor` folder contains elements that are used within the Unreal Editor only, e.g., UI components or functionality. These will not be included in a packaged build, so don't place anything here that is otherwise required for the application to run!

The `Runtime` folder contains everything else. The classes in here may be acted upon by the Unreal Editor, but they are not exclusive to the Editor.

Within the `Runtime` folder are two subfolders: `Public` and `Private`.

- The `Public` folder should contain files related to the public API.
- The `Private` folder contains everything else: private classes, functions, etc. that users will never have to deal with.

Be aware that the `Private` folder can reference files in the `Public` folder, but not vice versa. It's possible that a class in `Public` needs to `#include` something that would otherwise exist in the `Private` folder, e.g., the type of a private member variable. Use forward declarations to avoid this where possible, but you ultimately may have to move that class to the `Public` folder, even if it's not public API. This is fine.
