# API Design Guide

Cesium for Unreal aims to unlock geospatial capabilities in Unreal Engine, while also enabling developers to take advantage of in-engine features. As a result, people who use the plugin can often come with existing Unreal knowledge.

In order to make Cesium's integration as seamless as possible, we aim to mirror the paradigms and APIs of the engine where we can.

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

- Declare in the `private:` section of the class. This prevents the property from being get or set directly from outside the class in C++ (which is important, because there is no mechanism like `PostEditChangeProperty` or `BlueprintSetter` available in code).
- Add `Meta = (AllowPrivateAccess)`.
- Add `BlueprintGetter` and `BlueprintSetter` functions.
- Overide the `PostEditChangeProperty` method to be notified of changes to the property in the Editor.

Perhaps there is a rare-case property where no action is necessary after setting a property from C++ (and it's unlikely to be needed in the future). In this case, the property can be declared in the `public:` section. Such a property is not likely to need a `PostEditChangeProperty` or `BlueprintSetter` either.

#### Details Panel

### Cheat Sheet

For convenience, here is a cheat sheet of some of the most relevant modifiers for each category.

#### `UPROPERTY`

| Name | How (and when) to use |
| ---- | --------------------- |
| `VisibleAnywhere` | Property is read-only in the Details panel. For editable properties, use `EditAnywhere`. Don't use this for variables that shouldn't be visible to the user (e.g., implementation details, internal state management). |
| `EditAnywhere` | Property is editable in the Details panel. Use `VisibleAnywhere` for read-only variables. |
| `BlueprintReadOnly` | Property is accessible in Blueprints but read-only. |
| `BlueprintReadWrite` | Property is editable from Blueprints. Use when the "set" logic is simple enough that nothing additional must happen after the property is set. If additional logic is required, use `BlueprintSetter` instead. |

## Blueprints

Blueprints are a visual scripting option in Unreal Engine that many users use over C++ code. Thus, part of API design includes creating sensible Blueprints for less code-savvy users.

### Integrate with Existing Blueprints

Try to defer to Unreal Engine's naming schemes. For example, texture coordinates are referred to as "UV", so any parameters that represent texture coordinates should also be called UV.

![](Images/matchUnrealNaming.png)

## Materials

## Deprecation

## Backwards Compatibility