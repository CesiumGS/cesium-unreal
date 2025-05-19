# Developer Setup {#developer-setup-unreal}

## Overview

This is a summary of a setup and workflows for developers who want to work with the _Cesium for Unreal_ plugin. Such a setup consists of three main components:

- [`cesium-native`](https://github.com/CesiumGS/cesium-native) : A collection of engine-independent libraries for 3D Tiles, geospatial, etc. Most of the functionality of _Cesium for Unreal_ is built based on these libraries.
- [`cesium-unreal`](https://github.com/CesiumGS/cesium-unreal) : The source code of the actual _Cesium for Unreal_ plugin.
- An Unreal project that uses the plugin. We will use the [`cesium-unreal-samples`](https://github.com/CesiumGS/cesium-unreal-samples) as an example here, to get started quickly. It contains sample levels for different use cases, and can therefore be used to quickly check for possible regressions of feature changes.

> [!note]
> It is generally possible to work with `cesium-native` _independent_ of `cesium-unreal`. But any modification in `cesium-native` will have to be checked carefully for possible breaking changes in the API or the build process. So the following describes the developer setup from the perspective of someone who wants to work with `cesium-native` mainly in the context of `cesium-unreal`.

<!--! [TOC] -->

## Principles

There are detailed instructions for setting up a Cesium for Unreal development environment on each platform. But if you're already used to doing this sort of thing, you can probably use whatever workflow you like as long as you follow some important principles:

- To build Cesium for Unreal, you must first compile and cmake-install cesium-native and its dependencies to `Plugins/cesium-unreal/Source/ThirdParty`.
- cesium-native is built and installed using CMake and the `CMakeLists.txt` found in the `cesium-unreal/extern` directory, _not_ the one in the `cesium-unreal/extern/cesium-native` directory. When installing from this directory, the default install path will put cesium-native where Cesium for Unreal expects to find it.
- You must use the same compiler to build cesium-native and Cesium for Unreal.
- The Unreal project "DebugGame" configuration tries to use the debug build of cesium-native if one is built and installed, and falls back on the release build otherwise. The "Development" and "Shipping" configurations use the release build exclusively.
- Our CI build process checks for formatting using clang, and fails if code is improperly formatted. To run clang on all source code before committing, run `npm ci` to install node modules, then `npm run format`.

## Platform-specific setup instructions

<!--! \cond DOXYGEN_EXCLUDE !--> 
- [Windows](developer-setup-windows.md)
- [Linux](developer-setup-linux.md) 
- [macOS](developer-setup-osx.md)
<!--! \endcond -->
<!--! \li \subpage developer-setup-windows "Windows" -->
<!--! \li \subpage developer-setup-linux "Linux" -->
<!--! \li \subpage developer-setup-osx "macOS" -->

## Deployment instructions

- The [Packaging Guide](packaging-guide.md) describes how to create a package and use it in a local Unreal installation. This is similar to installing it from the Unreal Engine Marketplace, but allows testing non-release branches.
- The [Release Process](release-process.md) describes the necessary steps to publish a new version of the plugin to the Unreal Marketplace.

## Run the tests

- Open `cesium-unreal/TestsProject/TestsProject.uproject` in Unreal Engine
- From the menu, select Tools -> Test Automation
- In the Session Frontend Window, look for the Automation tab, and find the "Cesium" group of tests
- Check the Cesium group
- Click on "Start Tests"
> [!note]
> The TestsProject uses the Cesium and Functional Testing Editor plugins. You can run the tests from any project as long as you have both of these plugins enabled

## Generate Reference Documentation

- Install Doxygen and make sure `doxygen` is in your path.
- Run `npm install`
- Run `npm run doxygen`

The reference documentation will be written to `Documentation/Reference`.