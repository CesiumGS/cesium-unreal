# API Design Guide

Cesium for Unreal aims to unlock geospatial capabilities in Unreal Engine, while also enabling developers to take advantage of in-engine features. As a result, people who use the plugin can often come with existing Unreal knowledge.

In order to make Cesium's integration as seamless as possible, we aim to mirror the paradigms and APIs of the engine where we can. This guide provides best practices for how we do that.

## Table of Contents

- [UObject Design](#uobject-design)
  - [Overview](#overview)
  - [Best Practices](#best-practices)
  - [Cheat Sheet](#cheat-sheet)
- [Blueprints](#blueprints)

## UObject Design

`UObject` is the underlying base class of gameplay objects in Unreal Engine. Most classes in the Cesium for Unreal plugin inherit from `UObject`, such that they expose their functionality to the systems in Unreal Engine.

The [Overview](#overview) points out some of the components involved when designing `UObject`s. If you are already familiar with these, feel free to skip to the [Best Practices](#best-practices) section.

### Overview

#### `UObject`

> [Official Unreal Engine Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/objects-in-unreal-engine)

The official documentation already contains a summary of `UObject`, so we won't duplicate that here. However, the [UObject Creation](https://dev.epicgames.com/documentation/en-us/unreal-engine/objects-in-unreal-engine#uobjectcreation) and [Destroying Objects](https://dev.epicgames.com/documentation/en-us/unreal-engine/objects-in-unreal-engine#destroyingobjects) sections have a few elements of note:

- `UObject`s may only use default constructors. In other words, a `UObject` cannot have a constructor with arguments.
- For Actors or Actor Components, use the `BeginPlay()` method for specific initialization behavior.
- `UObject`s are automatically garbage collected when they are no longer referenced. Therefore, be mindful of "strong references" that can keep them alive (e.g., UProperties, class instances, `TStrongObjectPtr`).

#### Properties

> [Official Unreal Engine Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-uproperties)

`UObject`s often contain member variables (or properties) important to the object's gameplay logic. However, there is an extra step required to make Unreal Engine able to recognize and manipulate those variables. Once recognized, the properties can be acted upon, e.g., made accessible through Blueprints or the Editor UI.

The `UPROPERTY` macro identifies such properties. This macro should be put above a property that you wish to expose to Unreal Engine.

#### Functions

> [Official Unreal Engine Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/ufunctions-in-unreal-engine)

`UFUNCTION` is the equivalent of `UPROPERTY` for C++ functions. By using this macro, a function can be made access to Blueprints, or even as a button in the Editor interface. It may also be used to enable event handlers and delegates.

#### Enums

`UENUM` (TODO, short)

#### Modifiers

Many `UObject` macros take arguments that influence their scope and behavior. For example, let's look at the `MaximumScreenSpaceError` property on `ACesium3DTileset`:

```c++
  UPROPERTY(
      EditAnywhere,
      BlueprintGetter = GetMaximumScreenSpaceError,
      BlueprintSetter = SetMaximumScreenSpaceError,
      Category = "Cesium|Level of Detail",
      meta = (ClampMin = 0.0))
  double MaximumScreenSpaceError = 16.0;
  ```

- The `EditAnywhere` modifier allows the property to show up in the Details panel of the Unreal Editor. The user can thus modify the value directly.
- The `BlueprintGetter` and `BlueprintSetter` define specific functions for getting and setting the value in Blueprints (see [UFunctions](#ufunctions)).
- The `Category` indicates how the property should be organized in the Details panel. It appears under the "Level of Detail" category, which itself is under a larger "Cesium" category.
- Finally, `meta` refers to additional metadata that can augment how the property functions or appears. Here, `ClampMin` prevents it from being set to an invalid value.

The official documentation for `UPROPERTY` explains the fundamentals, but it is not comprehensive. This [masterlist](https://benui.ca/unreal/uproperty/) of `UPROPERTY` modifiers by ben ui provides a more extensive look into what's possible. 

The `UFUNCTION` macro also takes multiple arguments to influence its behavior. For instance,

```c++
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      meta = (ReturnDisplayName = "UnrealPosition"))
  FVector TransformLongitudeLatitudeHeightPositionToUnreal(
      const FVector& LongitudeLatitudeHeight) const;
```

- The `BlueprintPure` modifier allows the function to be executed in a Blueprint graph. The `Pure` keyword indicates that the function will not affect the owning object (`ACesiumGeoreference`) in any way.
- The `Category` indicates how the property should be organized in Blueprint selection panel, under "Cesium" category.
- Finally, `meta` refers to additional metadata that can augment how the property functions or appears. Here, `ReturnDisplayName` specifies how the output will be labelled in the Blueprint node.

`UFUNCTION` must be used for any functions that are used for `BlueprintGetter` or `BlueprintSetter`. For `BlueprintSetter`, the function should be `public` and serve as the mechanism for setting the property from C++. A corresponding `BlueprintGetter` is usually needed for use by C++, even though it is often not needed for Blueprints.

Again, the official documentation for `UFUNCTION` explains the fundamentals, but this [masterlist](https://benui.ca/unreal/uproperty/) of `UFUNCTION` modifiers by ben ui is more extensive. 

The [Cheat Sheet](#cheat-sheet) is included so you can focus on the most relevant ones.

### Best Practices

#### Change Detection

Many Cesium for Unreal classes manage an internal state that must be carefully maintained when modifying properties. For instance, when properties on the `ACesiumGeoreference` are changed, it must call `UpdateGeoreference()` immediately after to ensure that all other properties are synced in response.

As a result, `BlueprintReadWrite` is rarely used for properties in Cesium for Unreal. Much of the time, there is additional logic needed after a property is "set", and this change needs to be specifically detected.

We have settled on the following standards for properties that require post-change logic:

1. Declare properties as `private` in the class. This prevents the property from being get or set directly from outside the class in C++ (which is important, because there is no mechanism like `PostEditChangeProperty` or `BlueprintSetter` available in code).
2. Add `Meta = (AllowPrivateAccess)` to the `UPROPERTY`.
3. Add `BlueprintGetter` and `BlueprintSetter` functions.
4. Override the `PostEditChangeProperty` method on the class. This allows it to be notified of changes to the property in the Editor.

There may be a rare case where no action is necessary after setting a certain property from C++. In this case—and as long as this behavior is unlikely to change—the property can be declared in the `public:` section. Such a property is not likely to need `PostEditChangeProperty` or `BlueprintSetter` either.

#### Details Panel

Properties should be organized or modified in the Details panel such that they provide an intuitive user experience. There are many modifiers that facilitate this, also listed in the cheat sheet.

In general, be mindful of how properties are organized, and in what order. By default, properties will appear in the order that they are listed in the C++ code. A good principle is to start with properties that are fundamental to the functionality, then cascade into more advanced settings.

Properties should be organized into sensible categories, or as much as is reasonably possible, using the `Category` modifier. They should also be grouped together in C++ to reinforce this.

```c++

```

Often times, a `UObject` will have various properties that apply depending on the state of other properties. In this case, the `Meta=EditCondition` modifier is should be used for visual clarity. The dependent properties should be listed below the one that controls whether they're active.

Typically, such settings should depend on either a boolean setting or an enum setting. For instance, in `Cesium3DTileset`, the `Source` property affects whether the URL property can be edited. The `Enable Culling` flag

### Cheat Sheet

For convenience, here is a cheat sheet of some of the most relevant modifiers for each category.

#### `UPROPERTY`

| Name | What | When to Use |
| ---- | ----- | --------------- |
| `VisibleAnywhere` | Property is read-only in the Details panel. | Use for read-only variables. Don't use this for variables that shouldn't be visible to the user (e.g., implementation details, internal state management). |
| `EditAnywhere` | Property is editable in the Details panel. | Use for properties that should be user-editable through the Editor UI. |
| `BlueprintReadOnly` | Property is accessible in Blueprints but read-only. | Self-explanatory. | 
| `BlueprintReadWrite` | Property is editable from Blueprints. | Use when the "set" logic is simple enough that nothing additional must happen after the property is set. If additional logic is required, use `BlueprintSetter` instead. |
| `BlueprintGetter` | Property uses the specified function to get the value. | Use whenever you have to use `BlueprintSetter`. | 
| `BlueprintSetter` | Property uses the specified function to set the value. | Use whenever you have to do additional work after setting a property, e.g., recomputing the object's internal state. |

#### `UFUNCTION`

| Name | What | When to Use |
| ---- | ----- | --------------- |
| `BlueprintPure` | Function will be executed in a Blueprint graph without affect the owning object in any way. | Use for static Blueprints functions. |
| `meta = (ReturnDisplayName = "")` | Function output on the Blueprint node will be labeled with the specified name. | Use in general. |

## Blueprints 

Blueprints are a visual scripting option in Unreal Engine that many users use over C++ code. Thus, part of the API design in Cesium for Unreal includes creating sensible Blueprints for less code-savvy users.

### Copy Parameter Names

Try to defer to Unreal Engine's naming schemes for existing Blueprint functions and parameters. For example, texture coordinates in Blueprints are often referred to as "UV". Cesium for Unreal tries to match this by naming its own texture coordinate parameters as "UV".

![](Images/matchUnrealNaming.png)

## Deprecation and Backwards Compatibility

Ideally, APIs should not change frequently so that users aren't left confused and frustrated as they upgrade versions. But changes do happen from time to time. Thankfully, there are several measures in Unreal Engine to help prevent user frustration from API changes.

### Macros and Modifiers

First, read this short and sweet [overview](https://squareys.de/blog/ue4-deprecating-symbols/) by Jonathan Hale that explains how to deprecate anything in Unreal Engine. This section expands briefly on some points, but most of it is already covered.

- Use the `DeprecationMessage` should succinctly inform the user of the deprecation and redirect them to its replacement, if applicable.

```c++
  UFUNCTION(
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "CesiumMetadataPrimitive is deprecated. Get the associated property texture indices from CesiumPrimitiveMetadata instead."))
  static const TArray<FString>
  GetFeatureTextureNames(UPARAM(ref)
                             const FCesiumMetadataPrimitive& MetadataPrimitive);
```

- If a `struct` or `class` is deprecated, prefer to use the `UE_DEPRECATED` macro in a forward declaration of the class before it is actually defined. For example:

```c++
// Forward declare the class with the UE_DEPRECATED macro.
struct UE_DEPRECATED(
    5.0,
    "FCesiumMetadataPrimitive is deprecated. Instead, use FCesiumPrimitiveFeatures and FCesiumPrimitiveMetadata to retrieve feature IDs and metadata from a glTF primitive.")
    FCesiumMetadataPrimitive;

// Actual definition below.
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataPrimitive { ... }
```

- For backwards compatibility, sometimes you'll need to keep references to the deprecated classes or functions in C++ code. If this is the case, then be sure to wrap the relevant lines in `PRAGMA_DISABLE_DEPRECATION_WARNINGS` and `PRAGMA_ENABLE_DEPRECATION_WARNINGS`. This will reduce the spam in the Unreal logs that can otherwise occur during the build process.

```c++
struct LoadPrimitiveResult { 
  // List of properties here...

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  // For backwards compatibility with CesiumEncodedMetadataComponent.
  FCesiumMetadataPrimitive Metadata_DEPRECATED{};
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  // Other properties here...
};
```

### Core Redirects

> [Official Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/core-redirects-in-unreal-engine).

Whether you're renaming a class, function, property, or even an enum, it is possible to link between the old and new names to preserve existing Blueprint scripts. This is done using the `[CoreRedirects]` item in `Config/Engine.ini`.

One note: ensure that any entries are on the same line, otherwise they will not be parsed correctly.

### Versioning

