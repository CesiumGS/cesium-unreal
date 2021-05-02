# Cesium for Unreal Development Guide

This guide walks you through setting up a development environment for Cesium for Unreal with Cesium Native.

The following steps detail how to build the plugin and use it as part of your projects. You can also compile Cesium for Unreal as part of the [`cesium-unreal-samples`](https://github.com/CesiumGS/cesium-unreal-samples.git).

Cesium for Unreal depends on Cesium's high-precision geospatial C++ library - [Cesium Native](https://github.com/CesiumGS/cesium-native), which is included as a submodule.

## Compiling Cesium for Unreal

### Windows

#### Prerequisities

* Unreal Engine 4.26 or newer.
* Visual Studio 2017 or newer.
* Common development tools like Git and CMake.

#### Build Steps

1. Clone the repository using `git clone --recursive git@github.com:CesiumGS/cesium-unreal.git`.
2. From the `cesium-unreal/extern` directory, run the following commands to build `cesium-native`.

    ```cmd
    cmake -B build -S . -G "Visual Studio 15 2017 Win64" # Optionally use "Visual Studio 16 2019"
    cmake --build build --config Release --target install # Can optionally compile with --config RelWithDebInfo or MinSizeRel.
    cmake --build build --config Debug --target install # Optional, recommended for debugging
    ```

3. Point your Unreal Engine Project to the `CesiumForUnreal.uplugin` file to load the plugin into Unreal Engine.

### Mac OSX

#### Prerequisities

* Unreal Engine 4.26 or newer.
* XCode 11.3.1 (recommended) or newer
* Common development tools like Git and CMake.

#### Build Steps

1. Clone the repository using `git clone --recursive git@github.com:CesiumGS/cesium-unreal.git`.
2. From the `cesium-unreal/extern` directory, run the following commands to build `cesium-native`.

    ```bash
    cmake -B build -S . -DCMAKE_BUILD_TYPE=Release # or Debug, RelWithDebInfo
    cmake --build build --target install # Can optionally compile with --config RelWithDebInfo or MinSizeRel.
    ```

3. Point your Unreal Engine Project to the `CesiumForUnreal.uplugin` file to load the plugin into Unreal Engine.

### Linux

#### Prerequisities

* Unreal Engine 4.26 or newer. Follow the [Unreal Engine Linux Quikstart Guide](https://docs.unrealengine.com/en-US/SharingAndReleasing/Linux/BeginnerLinuxDeveloper/SettingUpAnUnrealWorkflow/index.html) to compile Unreal Engine on Linux.
* Common development tools like Git and CMake.

#### Build Steps

1. Compile Unreal Engine and set the following environment variables either temporary in your terminal or in `.bashrc`.

    ```cmd
    export UNREAL_ENGINE_DIR=<path_to_unreal_engine>
    export UNREAL_ENGINE_COMPILER_DIR=$UNREAL_ENGINE_DIR/Engine/Extras/ThirdPartyNotUE/SDKs/HostLinux/Linux_x64/v17_clang-10.0.1-centos7/x86_64-unknown-linux-gnu
    export UNREAL_ENGINE_LIBCXX_DIR=$UNREAL_ENGINE_DIR/Engine/Source/ThirdParty/Linux/LibCxx
    ```

2. Clone the repository using `git clone --recursive git@github.com:CesiumGS/cesium-unreal.git`.
3. From the `cesium-unreal/extern` directory, run the following commands to build `cesium-native`.

    ```bash
    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="unreal-linux-toolchain.cmake" -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Release # or Debug, RelWithDebInfo
    ```

4. Compile Cesium Native using `cmake --build build --target install`.

5. Point your Unreal Engine Project to the `CesiumForUnreal.uplugin` file to load the plugin into Unreal Engine.

### Android

#### Prerequisites for Cross Compiling on Linux

* Unreal Engine 4.26 or newer. Follow the [Unreal Engine Linux Quikstart Guide](https://docs.unrealengine.com/en-US/SharingAndReleasing/Linux/BeginnerLinuxDeveloper/SettingUpAnUnrealWorkflow/index.html) to compile Unreal Engine on Linux.
* Android NDK r21 (r21e recommended), download from https://developer.android.com/ndk/downloads.
* Common development tools like Git and CMake.

#### Cross Compiling on Linux

1. Compile Unreal Engine and set the following environment variables either temporary in your terminal or in `.bashrc`.

    ```cmd
    export ANDROID_NDK_ROOT=<path_to_android_ndk>
    ```

2. Clone the repository using `git clone --recursive git@github.com:CesiumGS/cesium-unreal.git`.
3. From the `cesium-unreal/extern` directory, run the following commands to build `cesium-native`.

    ```bash
    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="unreal-android-toolchain.cmake" -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Release # or Debug, RelWithDebInfo
    ```

4. Compile Cesium Native using `cmake --build build --target install`.

5. Point your Unreal Engine Project to the `CesiumForUnreal.uplugin` file to load the plugin into Unreal Engine.
