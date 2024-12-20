# Developer Setup for macOS {#developer-setup-osx}

Detailed instructions for setting up a Cesium for Unreal development environment on macOS. Please see the [Developer Setup](developer-setup.md) page for an overview of the process.
<!--! [TOC] -->

# Prerequisities

- Install CMake (version 3.15 or newer) from https://cmake.org/install/
- Install Xcode 11.3 from https://developer.apple.com/xcode/resources/
- For best JPEG-decoding performance, you must have [nasm](https://www.nasm.us/) installed so that CMake can find it. Everything will work fine without it, just slower.
- Install the Unreal Engine (version 4.26 or newer) from https://www.unrealengine.com/en-US/download

# Cloning the git repos

The following illustrates the recommended directory layout for developers:

- `~/dev` - Your own root directory for development.
- `~/dev/cesium-unreal-samples` - The directory for the Unreal project that will use the plugin.
- `~/dev/cesium-unreal-samples/Plugins/cesium-unreal` - The directory for the actual _Cesium for Unreal_ plugin.
- `~/dev/cesium-unreal-samples/Plugins/cesium-unreal/extern/cesium-native` - The directory for the base libraries project.

You may use any directory for the project, but the directory for the actual _Cesium for Unreal_ plugin **MUST** be in a subdirectory `Plugins/cesium-unreal` of the project directory. This way, Unreal will automatically find the Plugin when running the project, and pick up any changes that have been made to the plugin.

This can be set up with the following sequence of commands, on the console, starting in the `~/dev` directory:
```
git clone https://github.com/CesiumGS/cesium-unreal-samples.git
cd cesium-unreal-samples
mkdir Plugins
cd Plugins
git clone --recursive https://github.com/CesiumGS/cesium-unreal.git
```
<!--!\cond DOXYGEN_EXCLUDE !-->> Note:<!--! \endcond --><!--! \note --> The last line will also check out the `cesium-native` submodule and its dependencies. If you forget the `--recursive` option, you will see many compiler errors later in this process. If this happens to you, run the following in the `Plugins\cesium-unreal` directory to update the submodules in the existing clone:
```
git submodule update --init --recursive
```

# Building cesium-native

The cesium-native libraries and their dependencies use CMake and must be built separately from Cesium for Unreal. There are a number of ways to do this, but typically on macOS.

## CMake command-line

First, configure the CMake project in the `~/dev/cesium-unreal-samples/Plugins/cesium-unreal/extern` directory by following the instructions below.
<!--!\cond DOXYGEN_EXCLUDE !-->> Note:<!--! \endcond --><!--! \note --> The following steps must be done in the `extern` directory, and _not_ the `cesium-native` subdirectory!

Change to the `~/dev/cesium-unreal-samples/Plugins/cesium-unreal/extern` directory, and execute the following commands to build and install a Debug version of cesium-native:
```
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target install
```

To build a Release version, do the following:
```
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --target install
```

## CMake command-line for iOS

**Note**: It is recommended that the build steps for CMake command-line for macOS (above) be completed first. Unreal Engine Editor will not launch without the host side binaries compiled as well.

Configure the CMake project in the `~/dev/cesium-unreal-samples/Plugins/cesium-unreal/extern` directory by following the instructions below. Use a different build directory than the one use for macOS as this will require compiling for a different architecture.
<!--!\cond DOXYGEN_EXCLUDE !-->> Note:<!--! \endcond --><!--! \note -->  The following steps must be done in the `extern` directory, and _not_ the `cesium-native` subdirectory!

Change to the `~/dev/cesium-unreal-samples/Plugins/cesium-unreal/extern` directory, and execute the following commands to build and install a Release version of cesium-native:
```
cmake -B build-ios -S . -GXcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-ios --target install --config Release
```

You can also build and install the debug version by using `Debug` or `RelWithDebInfo` instead of `Release`.
