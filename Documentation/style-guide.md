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
- [File Organization](#file-organization)

## Naming

First, check out the [Naming Conventions](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine#namingconventions) section in the Unreal Engine coding standard. Then, read below.

### Unreal Prefixes

You will have to Unreal's conventions for naming new structs or classes, e.g, prefixing structs with `F` or actor subclasses with `A`. If these prefixes are absent, the Unreal Header Tool will likely complain during the build process, and subsequently fail to compile your code.

Although the names should be prefixed in code, the files containing them should *not* follow this rule. For example, `ACesium3DTileset` is defined in `Cesium3DTileset.h` and implemented in `Cesium3DTileset.cpp`, and *not* `ACesium3DTileset.h` or `ACesium3DTileset.cpp`.

### Public vs. Private

Functions and fields in the public API should be written in `PascalCase`. However, prefer to use `lowerCamelCase` for any private members or anonymous namespace functions.

```c++
USTRUCT()
struct CesiumStruct {
    public:
    float PublicField;
    void DoSomething(float Input);

    private:
    float _privateField;
    void doSomethingPrivate(float input);
}
```

When any pointers are used in the public API—whether as public fields or as parameters for functions—omit the `p` prefix. For instance:

```c++
// Don't do this.
UCesiumEllipsoid* pEllipsoid;
void SetEllipsoid(UCesiumEllipsoid* pNewEllipsoid)

// Do this.
UCesiumEllipsoid* Ellipsoid;
void SetEllipsoid(UCesiumEllipsoid* NewEllipsoid)
```

Finally, every `struct` or `class` in the public API should be prefaced with the word "Cesium". This helps to clearly separate our API from other elements in Unreal, and allows users to more easily search for our components in Unreal.

```c++
// Don't do this.
public AGeoreference : public AActor {...}

// Do this.
public ACesiumGeoreference : public AActor {...}
```

Note that the above is not enforced for classes used in private implementations. For example, users won't ever interact directly with the `LoadModelResult` or `UnrealAssetAccessor` classes, so there is no need to add a "Cesium" prefix.

### Clarity

Functions and variables should be assigned names that are descriptive yet succinct. This is important for public-facing API, but even private code should aspire to this standard.

**When in doubt, err on the side of being overly explicit.** Aim to be precise about scope and meaning.

For example, consider the context of multiple frames of reference in Cesium for Unreal. The plugin has to operate in multiple coordinate systems: Unreal's coordinate system, Earth-Centered Earth-Fixed coordinates, and cartographic coordinates (longitude, latitude, height). Whenever something deals with position or orientation, it is important to distinguish what space it is operating in.

For instance, this intentionally vague example leaves a lot to be desired. From a glance, it is not clear what the intended frame-of-reference is. The function body might tell us otherwise, but the declaration itself could afford more clarity.

```c++
void MoveToLocation(FVector Location) {...}
```

Adding documentation will definitely help here, and in some cases that will be enough to dispel ambiguity.

```c++
/**
* Moves the object to the given cartographic position, where
* - x = longitude
* - y = latitude
* - z = height above the WGS84 ellipsoid in meters.
*/
void MoveToLocation(FVector Location) {...}
```

However, we can take this even a step further. Let's rename the function and its parameter so that the intent is very obvious from the start.

```c++
/**
* Moves the object to the given cartographic position, where
* - x = longitude
* - y = latitude
* - z = height above the WGS84 ellipsoid in meters.
*/
void MoveToLongitudeLatitudeHeight(FVector LongitudeLatitudeHeight) {...}
```

For reference, here is what the counterpart for Unreal coordinate system might look like.

```c++
/**
* Moves the object to the given position in Unreal's coordinate system.
*/
void MoveToUnrealPosition(FVector UnrealPosition) {...}
```

Finally, in the same vein, avoid abbreviations unless they are commonly accepted. This improves readability, such that code is understandable to new developers from a first glance. Additionally, consider developers for whom English is not their first language; those arbitrary abbreviations can be even more confusing.

In general, it is good practice for code to read close to plain English. It does not have to be *verbose* English, but it should be succinctly and sufficiently understandable from a glance.

``` c++

// You might know what this means from a glance, but it can be jarring for newcomers.
static FQuat CalcRotAtLoc(FVector UELoc) {...}

// Better!
static FQuat CalculateRotationAtLocation(FVector UnrealLocation) {...}
```

### Units

Ensure that units stay consistent across the API to avoid confusion as values are passed. In particular:

- Although Unreal uses centimeter units, Cesium for Unreal uses meters, which is standard across our runtimes.
- For cartographic positions, longitude and latitude are expressed in degrees. Height above the WGS84 ellipsoid is expressed in meters.

It's encouraged to leave documentation about the expected units and/or frame of reference for a function and its parameters. This comment in `CesiumGeoreference.h` is a good example:

```c++
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

## File Organization

Cesium for Unreal's source code (located in the `Source` folder) is divided into `Runtime` and `Editor`. The `Editor` folder contains elements that are used within the Unreal Editor only, e.g., UI components or functionality. These will not be included in a packaged build, so don't place anything here that is otherwise required for the application to run!

The `Runtime` folder contains everything else. The classes in here may be acted upon by the Unreal Editor, but they are not exclusive to the Editor.

Within the `Runtime` folder are two subfolders: `Public` and `Private`.

- The `Public` folder should contain files related to the public API.
- The `Private` folder contains everything else: private classes, functions, etc. that users will never have to deal with.

Be aware that the `Private` folder can reference files in the `Public` folder, but not vice versa. It's possible that a class in `Public` needs to `#include` something that would otherwise exist in the `Private` folder, e.g., the type of a private member variable. Use forward declarations to avoid this where possible, but you ultimately may have to move that class to the `Public` folder, even if it's not public API. This is fine.