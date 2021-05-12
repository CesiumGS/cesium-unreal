Detailed instructions for setting up a Cesium for Unreal development environment on Linux. Please see the [Developer Setup](developer-setup.md) page for an overview of the process.

# Prerequisities

- Install CMake (version 3.15 or newer) from https://cmake.org/install/
- Build Unreal Engine for Linux by following the [Linux Quickstart](https://docs.unrealengine.com/en-US/SharingAndReleasing/Linux/BeginnerLinuxDeveloper/SettingUpAnUnrealWorkflow/index.html)

# Cloning the git repos

The following illustrates the recommended directory layout for developers:

- `~/dev` - Your own root directory for development.
- `~/dev/cesium-unreal-samples` - The directory for the Unreal project that will use the plugin.
- `~/dev/cesium-unreal-samples/Plugins/cesium-unreal` - The directory for the actual *Cesium for Unreal* plugin.
- `~/dev/cesium-unreal-samples/Plugins/cesium-unreal/extern/cesium-native` - The directory for the base libraries project.

You may use any directory for the project, but the directory for the actual *Cesium for Unreal* plugin **MUST** be in a subdirectory `Plugins/cesium-unreal` of the project directory. This way, Unreal will automatically find the Plugin when running the project, and pick up any changes that have been made to the plugin.

This can be set up with the following sequence of commands, on the console, starting in the `~/dev` directory:

    git clone https://github.com/CesiumGS/cesium-unreal-samples.git
    cd cesium-unreal-samples
    mkdir Plugins
    cd Plugins
    git clone --recursive https://github.com/CesiumGS/cesium-unreal.git

> Note: The last line will also check out the `cesium-native` submodule and its dependencies. If you forget the `--recursive` option, you will see many compiler errors later in this process. If this happens to you, run the following in the `Plugins\cesium-unreal` directory to update the submodules in the existing clone:

     git submodule update --init --recursive

# Building cesium-native

The cesium-native libraries and their dependencies use CMake and must be built separately from Cesium for Unreal. There are a number of ways to do this, but typically on Linux this is done with CMake from the command-line.

## CMake command-line

First, configure the CMake project in the `~/dev/cesium-unreal-samples/Plugins/cesium-unreal/extern` directory by following the instructions below.
**Note**: The following steps must be done in the `extern` directory, and *not* the `cesium-native` subdirectory!

First, set the environment variables used by the build process:

      export UNREAL_ENGINE_DIR=<path_to_unreal_engine>
      export UNREAL_ENGINE_COMPILER_DIR=$UNREAL_ENGINE_DIR/Engine/Extras/ThirdPartyNotUE/SDKs/HostLinux/Linux_x64/v17_clang-10.0.1-centos7/x86_64-unknown-linux-gnu
      export UNREAL_ENGINE_LIBCXX_DIR=$UNREAL_ENGINE_DIR/Engine/Source/ThirdParty/Linux/LibCxx


Change to the `~/dev/cesium-unreal-samples/Plugins/cesium-unreal/extern` directory, and execute the following commands to build and install a Debug version of cesium-native:

      cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="unreal-linux-toolchain.cmake" -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Debug
      cmake --build build --target install

To build a Release version, do the following:

      cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="unreal-linux-toolchain.cmake" -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Release
      cmake --build build --target install

# Creating the project files for the Unreal Engine game/project and the Cesium for Unreal plugin

Instructions coming soon. On Linux, projects should probably be built from the command-line by using RunUAT.
