# Cesium for Unreal Style Guide

Cesium strives to maintain a high standard for code quality across all of its codebases. The effort to write readable, organized, and consistent code makes it much easier for contributors to work with the code, whether new or experienced.

That said, the Cesium for Unreal plugin is a bit special. It combines code from [Cesium Native](https://github.com/CesiumGS/cesium-native/) and Unreal Engine, two large codebases with diverging guidelines, libraries, and design decisions. This guide attempts to reconcile between these two codebases and make recommendations where there are conflicts.

Before diving into this guide, we recommend the following:

1. Browse Cesium Native's [C++ style guide](https://github.com/CesiumGS/cesium-native/blob/main/doc/style-guide.md) to familiarize yourself with fundamental principles and expectations for C++ development.

2. Go through the [Unreal Engine Coding Standard](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine) for the official coding guidelines for Unreal Engine.

This guide will explicitly link to sections in the above resources when relevant, but it is good practice to learn the overall expectations of both environments.

**IN GENERAL**, if you're writing code for a public API, defer to Unreal Engine's style and code standards. When working with private APIs or implementation details, fall back on Cesium Native's C++ style. Use existing, similar code to infer the appropriate style for your own.

## Table of Contents

- [Types](#types)
- [Naming](#naming)
- [File Organization](#file-organization)
- [Macros](#macros)

(TODO)

## Types

### Pointers

Unreal provides engine-specific pointer types that may not be obvious to distinguish from one another. This [community guide](https://dev.epicgames.com/community/learning/tutorials/kx/unreal-engine-all-about-soft-and-weak-pointers) goes into detail about the differences between these types. They are summarized for convenience below.

Generally, use these whenever you instantiate something that inherits from an Unreal type (e.g., `UObject`, `USceneComponent`):

| Type | Description |
| ---- | ----------- |
| `TObjectPtr` | An optional replacement for raw pointers. The type `T` should be complete and derived from `UObject`. |
| `TSoftObjectPtr` | Used in UProperties.|
| `TWeakObjectPtr` | Used to reference already instantiated objects. Will resolve to null if the object gets destroyed or garbage collected.|

Unreal also provides `TUniquePtr` and `TSharedPtr` definitions, equivalent to their `std` counterparts. Use these whenever the type `T` is a `UObject`. For everything else – especially structs and classes in Cesium Native – stick with `std`.

### Math

Unreal contains various vector and matrix classes that are functionally similar to the classes in `glm`, e.g., `FVector` for `glm::dvec3`, `FMatrix` for `glm::dmat4`.

However, if you ever find yourself doing any vector and matrix arithmetic, it is **strongly encouraged** to do operations with `glm` first. The computed result can later be copied into the corresponding Unreal Engine type.

This is especially recommended for matrix math. Previously, `FMatrix` made strong assumptions about the content of the matrix that could result in unexpected behaviors. It also defined operators in a way that deviates from typical linear algebra expectations. If you need the result to be an `FMatrix`, first use `glm::dmat4` for the computation, and *then* copy it into an `FMatrix`.

> Example of faulty `FMatrix` math:
>
> The `FMatrix::operator*` composed matrices in the opposite of the normal order. If you wanted a matrix that transforms by `A` then by `B`, Unreal would want you to multiply `A * B`. You would otherwise multiply `B * A` in `glm`.
>
> (This may no longer be true, but it has ultimately proven safer to use `glm::dmat4` over Unreal's built-in matrix.)

## Naming

First, check out the [Naming Conventions](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine#namingconventions) section in the Unreal Engine coding standard. Then, read below.

### Prefixes

- Follow Unreal's conventions for naming new structs or classes, e.g, prefixing structs with `F` or actor subclasses with `A`. If these prefixes are absent, the Unreal Header Tool will likely complain during the build process, and subsequently fail to compile your code.

- Although the names should be prefixed in the code, the files containing them should *not* follow this rule. For example, `ACesium3DTileset` is defined in `Cesium3DTileset.h` and implemented in `Cesium3DTileset.cpp`, and *not* `ACesium3DTileset.h` or `ACesium3DTileset.cpp`.


### Public API

Functions and fields in the public API should be written in `PascalCase`. However, use `lowerCamelCase` for any private members or anonymous namespace functions.

```c++
USTRUCT()
public CesiumStruct {
    public:
    float PublicField;
    void DoSomething(float Input);

    private:
    float _privateField;
    void doSomethingPrivate(float input);
}
```

For parameters that are pointers, omit the `p` prefix. For instance:

```c++
// Don't do this.


// Do this.
```

Additionally, preface every `struct` or `class` in the public API with the word "Cesium". This helps to clearly separate our API from other elements in Unreal, and allows users to more easily search for our components in Unreal.

```c++
// Don't do this.
public AGeoreference : public AActor {...}

// Do this.
public ACesiumGeoreference : public AActor {...}
```

## File Organization

Cesium for Unreal's source code (located in the `Source` folder) is divided into `Runtime` and `Editor`. The `Editor` folder contains elements that are used within the Unreal Editor only, e.g., UI components or functionality. These will not be included in a packaged build, so don't place anything here that is otherwise required for the application to run!

The `Runtime` folder contains everything else. The classes in here may be acted upon by the Unreal Editor, but they are not exclusive to the Editor.

Within the `Runtime` folder are two subfolders: `Public` and `Private`.

- The `Public` folder should contain files related to the public API.
- The `Private` folder contains everything else: private classes, functions, etc. that users will never have to deal with.

Be aware that the `Private` folder can reference files in the `Public` folder, but not vice versa. It's possible that a class in `Public` needs to `#include` something that would otherwise exist in the `Private` folder, e.g., the type of a private member variable. Use forward declarations to avoid this where possible, but you ultimately may have to move that class to the `Public` folder, even if it's not public API. This is fine.

## Macros

Unreal Engine uses several macros that we also use in our code. There is no dedicated page that explains the workings of these macros, but the most relevant ones are summarized below.

### Class-Level Macros

Most structs and classes use top-level macros depending on the type of class they are. Upon building the plugin, these macros are intercepted by the Unreal Header Tool and used to generate boilerplate code for us. This results in features like interoperability with the Blueprint system and `UObject` garbage collection.

We primarily deal with `USTRUCT()`, `UCLASS()`. This results in `.generated.h` files being auto-generated for our classes.

Unreal's build tools will alert you of this, but for the record, the `.generated.h` file should be the very last include at the top of the file.

## Units

Although Unreal uses centimeter units, Cesium for Unreal uses meters, which is standard across our runtimes. To avoid confusion, it is best to make remarks about the expected units of a function in its documentation. Be as consistent with scale.