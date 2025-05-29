# Developer Setup for Linux {#developer-setup-linux}

Detailed instructions for setting up a Cesium for Unreal development environment on Linux. Please see the [Developer Setup](developer-setup.md) page for an overview of the process.
<!--! [TOC] -->

# Prerequisities

- Install CMake (version 3.15 or newer) from https://cmake.org/install/
- For best JPEG-decoding performance, you must have [nasm](https://www.nasm.us/) installed so that CMake can find it. Everything will work fine without it, just slower.
- Build Unreal Engine for Linux by following the [Linux Quickstart](https://docs.unrealengine.com/en-US/SharingAndReleasing/Linux/BeginnerLinuxDeveloper/SettingUpAnUnrealWorkflow/index.html)

After compiling Unreal Engine, set the following environment variables in your `.bashrc`.

```bash
export UNREAL_ENGINE_DIR=<path_to_unreal_engine>
export UNREAL_ENGINE_COMPILER_DIR=$UNREAL_ENGINE_DIR/Engine/Extras/ThirdPartyNotUE/SDKs/HostLinux/Linux_x64/v20_clang-13.0.1-centos7/x86_64-unknown-linux-gnu
export UNREAL_ENGINE_LIBCXX_DIR=$UNREAL_ENGINE_DIR/Engine/Source/ThirdParty/Unix/LibCxx
```
> [!note]
> `v20_clang-13.0.1-centos7` is correct for Unreal Engine v5.0.3. It may be different for other versions of Unreal Engine. See [https://docs.unrealengine.com/5.0/en-US/SharingAndReleasing/Linux/GettingStarted/](https://docs.unrealengine.com/5.0/en-US/linux-development-requirements-for-unreal-engine/) or the equivalent for your version of Unreal Engine.

# Cloning the git repos

The following illustrates the recommended directory layout for developers:

- `~/dev` - Your own root directory for development.
- `~/dev/cesium-unreal-samples` - The directory for the Unreal project that will use the plugin.
- `~/dev/cesium-unreal` - The directory for the actual _Cesium for Unreal_ plugin.
- `~/dev/cesium-unreal/extern/cesium-native` - The directory for the base libraries project.

In this setup, we will build the Cesium for Unreal plugin separately from any project, and then install it as an Engine plugin.

First, let's clone the Cesium for Unreal repo by issuing the following command in the `~/dev` directory:
```
git clone -b ue5-main --recursive https://github.com/CesiumGS/cesium-unreal.git
```
> [!note]
> The last line will also check out the `cesium-native` submodule and its dependencies. If you forget the `--recursive` option, you will see many compiler errors later in this process. If this happens to you, run the following in the `Plugins\cesium-unreal` directory to update the submodules in the existing clone:

    git submodule update --init --recursive

# Building cesium-native

The cesium-native libraries and their dependencies use CMake and must be built separately from Cesium for Unreal. There are a number of ways to do this, but typically on Linux this is done with CMake from the command-line.

## CMake command-line

Configure the CMake project in the `~/dev/cesium-unreal/extern` directory by following the instructions below.
> [!note]
> The following steps must be done in the `extern` directory, and _not_ the `cesium-native` subdirectory!

Change to the `~/dev/cesium-unreal/extern` directory, and execute the following commands to build and install a Debug version of cesium-native:

    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="unreal-linux-toolchain.cmake" -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --target install

To build a Release version, do the following:
```
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="unreal-linux-toolchain.cmake" -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --target install
```
> [!note]
> To build faster by using multiple CPU cores, add `-j14` to the build/install command above, i.e. `cmake --build build --target install -j14`. "14" is the number of threads to use, and a higher or lower number may be more suitable for your system.

## KTX-Software workaround

You may encounter an issue with the Unreal Build tool generating errors with symlinks in the KTX-Software submodule. This directory is no longer needed after cesium-native is built, therefore it can safely be removed.

Delete the entire 'cesium-unreal/extern' directory:

    cd ..
    rm -rf extern

Alternatively, this directory could be moved elsewhere and brought back in the case of a rebuild.

# Build and package the Cesium for Unreal plugin

Store the absolute path of the root directory of the `cesium-unreal` repo in an environment variable for ease of reference:

    export CESIUM_FOR_UNREAL_DIR=~/dev/cesium-unreal # adjust path for your system

And build the plugin:

    cd $UNREAL_ENGINE_DIR/Engine/Build/BatchFiles
    ./RunUAT.sh BuildPlugin -Plugin="$CESIUM_FOR_UNREAL_DIR/CesiumForUnreal.uplugin" -Package="$CESIUM_FOR_UNREAL_DIR/../packages/CesiumForUnreal" -CreateSubFolder -TargetPlatforms=Linux

And finally copy the built plugin into the Engine plugins directory:
```
mkdir -p $UNREAL_ENGINE_DIR/Engine/Plugins/Marketplace
cp -r $CESIUM_FOR_UNREAL_DIR/../packages/CesiumForUnreal $UNREAL_ENGINE_DIR/Engine/Plugins/Marketplace/
```
> [!note]
> On Linux (unlike Windows), it is essential that the `CesiumForUnreal` plugin go in the `Plugins/Marketplace/` subdirectory, rather than directly in `Plugins/`. Otherwise, the relative paths to other plugin `.so` files that the Unreal Build Tool has built into the plugin will not resolve correctly.

# Using the plugin with the Cesium for Unreal Samples project

The Cesium for Unreal Samples project demonstrates a bunch of features of Cesium for Unreal, and it is useful during development for testing the plugin. I can be cloned from GitHub as well:

    cd ~/dev
    git clone https://github.com/CesiumGS/cesium-unreal-samples.git

Then, launch the Unreal Editor and open `~/dev/cesium-unreal-samples/CesiumForUnrealSamples.uproject`. Because we've already installed the plugin to the Engine Plugins directory, the samples project should pick it up automatically.