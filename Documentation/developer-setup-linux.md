Detailed instructions for setting up a Cesium for Unreal development environment on Linux. Please see the [Developer Setup](developer-setup.md) page for an overview of the process.

# Prerequisities

- Install CMake (version 3.15 or newer) from https://cmake.org/install/
- Build Unreal Engine for Linux by following the [Linux Quickstart](https://docs.unrealengine.com/en-US/SharingAndReleasing/Linux/BeginnerLinuxDeveloper/SettingUpAnUnrealWorkflow/index.html)

After compiling Unreal Engine, set the following environment variables in your `.bashrc`.

```bash
export UNREAL_ENGINE_DIR=<path_to_unreal_engine>
export UNREAL_ENGINE_COMPILER_DIR=$UNREAL_ENGINE_DIR/Engine/Extras/ThirdPartyNotUE/SDKs/HostLinux/Linux_x64/v19_clang-11.0.1-centos7/x86_64-unknown-linux-gnu
export UNREAL_ENGINE_LIBCXX_DIR=$UNREAL_ENGINE_DIR/Engine/Source/ThirdParty/Linux/LibCxx
```

> Note: `v19_clang-11.0.1-centos7` is correct for Unreal Engine v4.27 and v5.0.0. It may be different for other versions of Unreal Engine.

# Cloning the git repos

The following illustrates the recommended directory layout for developers:

- `~/dev` - Your own root directory for development.
- `~/dev/cesium-unreal-samples` - The directory for the Unreal project that will use the plugin.
- `~/dev/cesium-unreal` - The directory for the actual *Cesium for Unreal* plugin.
- `~/dev/cesium-unreal/extern/cesium-native` - The directory for the base libraries project.

In this setup, we will build the Cesium for Unreal plugin separately from any project, and then install it as an Engine plugin.

This can be set up with the following sequence of commands, on the console, starting in the `~/dev` directory:

    git clone https://github.com/CesiumGS/cesium-unreal-samples.git
    git clone -b ue4-main --recursive https://github.com/CesiumGS/cesium-unreal.git

> Note: The last line will also check out the `cesium-native` submodule and its dependencies. If you forget the `--recursive` option, you will see many compiler errors later in this process. If this happens to you, run the following in the `Plugins\cesium-unreal` directory to update the submodules in the existing clone:

    git submodule update --init --recursive

# Building cesium-native

The cesium-native libraries and their dependencies use CMake and must be built separately from Cesium for Unreal. There are a number of ways to do this, but typically on Linux this is done with CMake from the command-line.

## CMake command-line

First, configure the CMake project in the `~/dev/cesium-unreal/extern` directory by following the instructions below.
**Note**: The following steps must be done in the `extern` directory, and *not* the `cesium-native` subdirectory!

Change to the `~/dev/cesium-unreal/extern` directory, and execute the following commands to build and install a Debug version of cesium-native:

    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="unreal-linux-toolchain.cmake" -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --target install -j14

To build a Release version, do the following:

    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="unreal-linux-toolchain.cmake" -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target install -j14

# Build and package the Cesium for Unreal plugin

Store the absolute path of the root directory of the `cesium-unreal` repo in an environment variable for ease of reference:

    export CESIUM_FOR_UNREAL_DIR=~/dev/cesium-unreal # adjust path for your system

And build the plugin:

    cd $UNREAL_ENGINE_DIR/Engine/Build/BatchFiles
    ./RunUAT.sh BuildPlugin -Plugin="$CESIUM_FOR_UNREAL_DIR/CesiumForUnreal.uplugin" -Package="$CESIUM_FOR_UNREAL_DIR/../packages/CesiumForUnreal" -CreateSubFolder -TargetPlatforms=Linux

And finally copy the built plugin into the Engine plugins directory:

    mkdir -p $UNREAL_ENGINE_DIR/Engine/Plugins/Marketplace
    cp -r $CESIUM_FOR_UNREAL_DIR/../packages/CesiumForUnreal $UNREAL_ENGINE_DIR/Engine/Plugins/Marketplace/

> Note: On Linux (unlike Windows), it is essential that the `CesiumForUnreal` plugin go in the `Plugins/Marketplace/` subdirectory, rather than directly in `Plugins/`. Otherwise, the relative paths to other plugin `.so` files that the Unreal Build Tool has built into the plugin will not resolve correctly.

You should now be able to launch the Unreal Editor and open the `CesiumForUnrealSamples.uproject` found in `~/dev/cesium-unreal-samples`.
