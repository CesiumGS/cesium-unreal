## Overview

This is a summary of a setup and workflows for developers who want to work with the *Cesium for Unreal* plugin. Such a setup consists of three main components:

- [`cesium-native`](https://github.com/CesiumGS/cesium-native) : A collection of engine-independent libraries for 3D Tiles, geospatial, etc. Most of the functionality of *Cesium for Unreal* is built based on these libraries.
- [`cesium-unreal`](https://github.com/CesiumGS/cesium-unreal) : The source code of the actual *Cesium for Unreal* plugin.
- An Unreal project that uses the plugin. We will use the [`cesium-unreal-samples`](https://github.com/CesiumGS/cesium-unreal-samples) as an example here, to get started quickly. It contains sample levels for different use cases, and can therefore be used to quickly check for possible regressions of feature changes.

> Note: It is generally possible to work with `cesium-native` *independent* of `cesium-unreal`. But any modification in `cesium-native` will have to be checked carefully for possible breaking changes in the API or the build process. So the following describes the developer setup from the perspective of someone who wants to work with `cesium-native` mainly in the context of `cesium-unreal`.

## Principles

There are detailed instructions for setting up a Cesium for Unreal development environment on each platform. But if you're already used to doing this sort of thing, you can probably use whatever workflow you like as long as you follow some important principles:

- To build Cesium for Unreal, you must first compile and cmake-install cesium-native and its dependencies to `Plugins/cesium-unreal/Source/ThirdParty`.
- cesium-native is built and installed using CMake and the `CMakeLists.txt` found in the `cesium-unreal/extern` directory, _not_ the one in the `cesium-unreal/extern/cesium-native` directory. When installing from this directory, the default install path will put cesium-native where Cesium for Unreal expects to find it.
- You must use the same compiler to build cesium-native and Cesium for Unreal.
- On Windows, packaging the Cesium for Unreal plugin on Windows requires Visual Studio 2017. This means you must also build cesium-native with Visual Studio 2017 in order for plugin packaging to be successful. But if you're not concerned with packaging, you can use Visual Studio 2019 exclusively.
- The Unreal project "DebugGame" configuration tries to use the debug build of cesium-native if one is built and installed, and falls back on the release build otherwise. The "Development" and "Shipping" configurations use the release build exclusively.

## Platform-specific setup instructions

* [Windows](developer-setup-windows.md)
* [Linux](developer-setup-linux.md)
* macOS - coming soon
