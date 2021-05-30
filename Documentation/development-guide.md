# Cesium for Unreal Development Guide

This guide walks you through setting up a development environment for Cesium for Unreal with Cesium Native.

The following steps detail how to build the plugin and use it as part of your projects. You can also compile Cesium for Unreal as part of the [`cesium-unreal-samples`](https://github.com/CesiumGS/cesium-unreal-samples.git).

Cesium for Unreal depends on Cesium's high-precision geospatial C++ library - [Cesium Native](https://github.com/CesiumGS/cesium-native), which is included as a submodule.

**Table of Contents**

0. [Prerequisities](#0-prerequisities)
    * [Windows](#windows)
    * [Mac OSX](#mac-osx)
    * [Linux](#linux)
1. [Clone Cesium for Unreal](#1-clone-cesium-for-unreal)
2. [Compiling Cesium Native](#2-compiling-cesium-native)
    * [Windows](#windows-1)
    * [Mac OSX](#mac-osx-1)
    * [Linux](#linux-1)
    * [Android (Cross Compiling on Windows)](#android-cross-compiling-on-windows)
3. [Cesium for Unreal](#3-cesium-for-unreal)
    * [Using Cesium for Unreal from Local Directory](#using-cesium-for-unreal-from-local-directory)
    * [Packaging Cesium for Unreal Plugin](#packaging-cesium-for-unreal-plugin)

## 0. Prerequisities

### Windows

* Unreal Engine 4.26 or newer.
* Visual Studio 2017 or newer.
* Common development tools like Git and CMake.

#### Optional for Cross Compiling Android on Windows

* Android NDK r21, download from https://developer.android.com/ndk/downloads.

### Mac OSX

* Unreal Engine 4.26 or newer.
* XCode 11.3.1 (recommended) or newer
* Common development tools like Git and CMake.

### Linux

* Unreal Engine 4.26 or newer. Follow the [Unreal Engine Linux Quickstart Guide](https://docs.unrealengine.com/en-US/SharingAndReleasing/Linux/BeginnerLinuxDeveloper/SettingUpAnUnrealWorkflow/index.html) to compile Unreal Engine on Linux.
* Common development tools like Git and CMake.

After compiling Unreal Engine, set the following environment variables in your `.bashrc`.

```bash
export UNREAL_ENGINE_DIR=<path_to_unreal_engine>
export UNREAL_ENGINE_COMPILER_DIR=$UNREAL_ENGINE_DIR/Engine/Extras/ThirdPartyNotUE/SDKs/HostLinux/Linux_x64/v17_clang-10.0.1-centos7/x86_64-unknown-linux-gnu
export UNREAL_ENGINE_LIBCXX_DIR=$UNREAL_ENGINE_DIR/Engine/Source/ThirdParty/Linux/LibCxx
```

## 1. Clone Cesium for Unreal

If you are planning to build and package a standalone version of the Cesium for Unreal plugin, you can clone the repository using `git clone --recursive git@github.com:CesiumGS/cesium-unreal.git` to any workspace on your computer.

If you plan to use Cesium for Unreal as part of a specific GitHub project and plan to modify it, then clone the plugin as a subdirectory under the project's `Plugins` directory. For example, if using Cesium for Unreal samples, then clone Cesium for Unreal from `cesium-unreal-samples/Plugins`.

## 2. Compiling Cesium Native

### Windows

#### Build Steps

From the `cesium-unreal/extern` directory, run the following commands to build `cesium-native`.

```cmd
cmake -B build -S . -G "Visual Studio 15 2017 Win64" # Optionally use "Visual Studio 16 2019"
cmake --build build --config Release --target install # Can optionally compile with --config RelWithDebInfo or MinSizeRel.
cmake --build build --config Debug --target install # Optional, recommended for debugging
```

### Mac OSX

#### Build Steps

From the `cesium-unreal/extern` directory, run the following commands to build `cesium-native`.

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release # Can optionally compile with RelWithDebInfo or MinSizeRel.
cmake --build build --target install
```

### Linux

From the `cesium-unreal/extern` directory, run the following commands to build `cesium-native`.

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="unreal-linux-toolchain.cmake" -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Release  # Can optionally compile with RelWithDebInfo or MinSizeRel.
make --build build --target install
```

### Android (Cross Compiling on Windows)

1. Follow the steps for Windows above.
2. Download and extract the Android NDK zip. Then set the environment variable either in command line or system variables. Note that you must use forward-slashes, i.e. `c:/android` not `c:\android`.
    ```cmd
    SET ANDROID_NDK_ROOT=<path_to_android_ndk>
    ```
3. From the `cesium-unreal/extern` directory, run the following commands to build `cesium-native` for Android.
    ```cmd
    cmake -B build-android -S . -G Ninja -DCMAKE_TOOLCHAIN_FILE="unreal-android-toolchain.cmake" -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Release # or Debug, RelWithDebInfo
    cmake --build build-android --config Release --target install
    ```

## 3. Cesium for Unreal

Once you have compiled Cesium Native, you have the option to either:
* use it as a local plugin within your development workspace, or
* package Cesium for Unreal and copy it into the Engine directory.

### Using Cesium for Unreal from Local Directory

Using plugins from local project plugin directories can be useful when actively modifying the plugin. This can be done by moving (or cloning) Cesium for Unreal from your `Project/Plugins` directory.

For example, if you were using the Cesium for Unreal Samples project, you can clone Cesium for Unreal from the `cesium-unreal-samples/Plugins` directory. Unreal Engine can then pick up the the plugin as a local dependency for your Unreal project.

### Packaging Cesium for Unreal Plugin

Packaging an Unreal Engine plugin makes it portable and you can use it as part of the Unreal Engine directory, to be used for all your Unreal projects. This is similar to installing it from the Unreal Engine Marketplace, except you can use non-release branches.

To package the plugin, follow the steps below:

1. After compiling Cesium Native in your `cesium-unreal` directory, run the following command:
    ```bash
    # Command Template:
    $UNREAL_ENGINE_DIR/Engine/Build/BatchFiles/RunUAT.bat" BuildPlugin -Plugin="<absolute path to cesium-unreal/CesiumForUnreal.uplugin>" -Package="<absolute path to output directory>" -CreateSubFolder -TargetPlatforms=<target platforms>
    ```
    * Windows Example:
        ```cmd
        "C:\Program Files\Epic Games\UE_4.26\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin="C:\workspace\cesium-unreal\CesiumForUnreal.uplugin" -Package="C:\workspace\Packages\CesiumForUnreal" -CreateSubFolder -TargetPlatforms=Win64
        ```
    * Windows+Android Example:
        ```cmd
        "C:\Program Files\Epic Games\UE_4.26\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin="C:\workspace\cesium-unreal\CesiumForUnreal.uplugin" -Package="C:\workspace\Packages\CesiumForUnreal" -CreateSubFolder -TargetPlatforms=Win64+Android
        ```
    * Mac OSX Example:
        ```bash
        "$UNREAL_ENGINE_DIR/Engine/Build/BatchFiles/RunUAT.sh" BuildPlugin -Plugin="/home/user/workspace/cesium-unreal/CesiumForUnreal.uplugin" -Package="/home/user/workspace/packages/CesiumForUnreal" -CreateSubFolder -TargetPlatforms=Mac
        ```
    * Linux Example:
        ```bash
        "$UNREAL_ENGINE_DIR/Engine/Build/BatchFiles/RunUAT.sh" BuildPlugin -Plugin="/home/user/workspace/cesium-unreal/CesiumForUnreal.uplugin" -Package="/home/user/workspace/packages/CesiumForUnreal" -CreateSubFolder -TargetPlatforms=Linux
        ```
2. Copy the output directory `CesiumForUnreal` to the Unreal Engine Plugins directory, example `C:\Program Files\Epic Games\UE_4.26\Engine\Plugins\Marketplace\CesiumForUnreal` on Windows. Once the packged plugin is copied, all Unreal projects will be able to enable it.
    * Note: you can also copy the `CesiumForUnreal` directory to a specific projects `Plugins` directory, such as `cesium-unreal-samples/Plugins/CesiumForUnreal`. In this case, the built plugin is only available to the specific project and will be prioritized over Engine-level copies of Cesium for Unreal.
