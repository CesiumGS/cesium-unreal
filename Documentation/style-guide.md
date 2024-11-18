# Cesium for Unreal Style Guide

Cesium strives to maintain a high standard for code quality across all of its codebases. The effort to write readable, organized, and consistent code makes it much easier for contributors to work with the code, whether new or experienced.

That said, the Cesium for Unreal plugin is a bit special – it sits at the intersection of [Cesium Native](https://github.com/CesiumGS/cesium-native/) and Unreal Engine, two large codebases with diverging guidelines, libraries, and design decisions. This guide attempts to reconcile between these two codebases and give recommendations where there are conflicts of style.

Before diving into this guide, we recommend the following:

1. Browse Cesium Native's [C++ style guide](https://github.com/CesiumGS/cesium-native/blob/main/doc/style-guide.md) to familiarize yourself with fundamental principles and expectations for C++ development.

2. Go through the [Unreal Engine Coding Standard](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine) for the official coding guidelines for Unreal Engine.

This guide will explicitly link to sections in the above resources when relevant, but it is good practice to learn the overall expectations of both environments.

**IN GENERAL**, use the existing context and codebase to infer the appropriate style for your code. If you're writing code for a public API, defer to Unreal Engine's style and code standards. When working with private APIs or implementation details, fall back on Cesium Native's C++ style.

## Table of Contents

- [Naming](#naming)
- [File Organization](#file-organization)
- [Macros](#macros)
- [Math](#math)

(TODO)

## Naming

Unreal's source code uses naming conventions that we aim to follow in Cesium for Unreal. See the [Naming Conventions](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine#namingconventions) section in the Unreal Engine coding standard.

### Prefixes

- Follow Unreal's conventions for naming new structs or classes, e.g, prefixing structs with `F` or actor subclasses with `A`. If these prefixes are absent, the Unreal Header Tool will likely complain during the build process, and subsequently fail to compile your code.

- Although the names should be prefixed in the code, the files containing them should *not* follow this rule. For example, `ACesium3DTileset` is defined in `Cesium3DTileset.h` and implemented in `Cesium3DTileset.cpp`, and *not* `ACesium3DTileset.h` or `ACesium3DTileset.cpp`.

- Preface everything in the public API with the word "Cesium". This helps to clearly separate our API from other elements in Unreal, and allows users to more easily search for our components in Unreal.

```c++
// Bad
public AGeoreference : public AActor {...}

// Good
public ACesiumGeoreference : public AActor {...}
```

## File Organization

Cesium for Unreal's source code (located accordingly in the `Source` folder) is divided into two folders: `Runtime` and `Editor`.

- The `Editor` folder contains everything that is used within the Unreal Editor only. These elements will not be included in a packaged build, so don't leave anything here that is required for the application to run!
- The `Runtime` folder contains everything that is used within the application. These classes may be acted upon by the Unreal Editor, but are not exclusive to the Editor.

Within the `Runtime` folder are two subfoldres: `Public` and `Private`.

- The `Public` folder should contain files related to the public API.
- The `Private` folder contains everything else: private classes, functions, etc. that users will never have to deal with.

Be aware that the `Private` folder can reference files in the `Public` folder, but not vice versa. It's possible that a class in `Public` needs to `#include` something that would otherwise exist in the `Private` folder, e.g., the type of a private member variable. Use forward declarations to avoid this where possible, but you ultimately may have to move that class to the `Public` folder. This is fine.

## Macros

Unreal Engine uses several macros that we also use in our code. There is no dedicated page that explains the workings of these macros, but the most relevant ones are summarized below.

### Class-Level Macros

Most structs and classes use top-level macros depending on the type of class they are. Upon building the plugin, these macros are intercepted by the Unreal Header Tool and used to generate boilerplate code for us. This results in features like interoperability with the Blueprint system and `UObject` garbage collection.

We primarily deal with `USTRUCT()`, `UCLASS()`. This results in `.generated.h` files being auto-generated for our classes.

Unreal's build tools will alert you of this, but for the record, the `.generated.h` file should be the very last include at the top of the file.

### Render Thread Macros

`ENQUEUE_RENDER_COMMAND`.


## Units

Although Unreal uses centimeter units, Cesium for Unreal uses meters, which is standard across our runtimes. To avoid confusion, it is best to make remarks about the expected units of a function in its documentation. Be as consistent with scale.

## Math

Unreal provides vector and matrix classes that allow for linear algebra operations. It is acceptable to use this for the vector math, as this is relatively straight forward.

However, for matrix math it is strongly recommended to use `glm`. This is because `FMatrix` makes strong assumptions about the content of the matrix that could result in unexpected behaviors.

