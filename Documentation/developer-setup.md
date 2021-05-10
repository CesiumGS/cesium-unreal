## Overview

This is a summary of a setup and workflows for developers who want to work with the *Cesium for Unreal* plugin. Such a setup consists of three main components:

- [`cesium-native`](https://github.com/CesiumGS/cesium-native) : A collection of engine-independent libraries for 3D Tiles, geospatial, etc. Most of the functionality of *Cesium for Unreal* is built based on these libraries.
- [`cesium-unreal`](https://github.com/CesiumGS/cesium-unreal) : The source code of the actual *Cesium for Unreal* plugin.
- An Unreal project that uses the plugin. We will use the [`cesium-unreal-samples`](https://github.com/CesiumGS/cesium-unreal-samples) as an example here, to get started quickly. It contains sample levels for different use cases, and can therefore be used to quickly check for possible regressions of feature changes.

> Note: It is generally possible to work with `cesium-native` *independent* of `cesium-unreal`. But any modification in `cesium-native` will have to be checked carefully for possible breaking changes in the API or the build process. So the following describes the developer setup from the perspective of someone who wants to work with `cesium-native` mainly in the context of `cesium-unreal`.

## Prerequisities

- Install CMake (version 3.15 or newer) from https://cmake.org/install/
- Install an appropriate C++ compiler or IDE for your platform:
  - On Windows: Visual Studio 2017 v15.6+ or Visual Studio 2019 v16.5+.
  - On macOS: Xcode v11.3.1+
  - On Linux: Clang v10.0.1+
- Install the Unreal Engine (version 4.26 or newer) from https://www.unrealengine.com/en-US/download

## Initial setup 

### Setting up the directories

The following illustrates the recommended directory layout for developers:

- `C:\Dev` - Your own root directory for development.
- `C:\Dev\cesium-unreal-samples` - The directory for the Unreal project that will use the plugin.
- `C:\Dev\cesium-unreal-samples\Plugins\cesium-unreal` - The directory for the actual *Cesium for Unreal* plugin.
- `C:\Dev\cesium-unreal-samples\Plugins\cesium-unreal\extern\cesium-native` - The directory for the base libraries project.

You may use any directory for the project, but the directory for the actual *Cesium for Unreal* plugin **MUST** be in a subdirectory `Plugins/cesium-unreal` of the project directory. This way, Unreal will automatically find the Plugin when running the project, and pick up any changes that have been made to the plugin.

This can be set up with the following sequence of commands, on the console, starting in the `C:\Dev` directory:

    git clone https://github.com/CesiumGS/cesium-unreal-samples.git
    cd cesium-unreal-samples
    mkdir Plugins
    cd Plugins
    git clone --recursive https://github.com/CesiumGS/cesium-unreal.git

> Note: The last line will also check out the `cesium-native` submodule and its dependencies. If you forget the `--recursive` option, you will see many compiler errors later in this process. Run `git submodule update --init --recursive` from the `Plugins/cesium-unreal` directory to recover by updating the submodules in an existing clone.

### Building cesium-native

The cesium-native libraries and their dependencies use CMake and must be built separately from Cesium for Unreal. There are a number of ways to do this depending your preferred environment:

* Visual Studio 2019 (Windows)
* Visual Studio Code (Any Platform)
* CMake command-line (Any Platform)
* CMake GUI (Any Platform)

The version of CMake included with Visual Studio 2017 is too old to build cesium-native, so to build with Visual Studio 2017, follow the CMake command-line or CMake GUI instructions.

#### Building cesium-native with Visual Studio 2019

Visual Studio 2019 has built-in support for CMake as long as the "C++ CMake tools for Windows" option was selected at install time. It is included by default in the "Desktop Development with C++" workload.

To begin, launch Visual Studio 2019 and "Open a local folder". Select the `C:\Dev\cesium-unreal-samples\Plugins\cesium-unreal\extern`. Be sure to select the `extern` directory, *not* the `cesium-native` subdirectory.

Then, in the "Solution Explorer - Folder View", right-click on the root `CMakeLists.txt` and select "Install". This will compile a debug build of cesium-native and "install" it to the place in the project that Cesium for Unreal expects to find it.

To also build a "Release" build of cesium-native, right click on `CMakeLists.txt` and select "CMake Settings for cesium-unreal-extern". Add a new configuration by clicking the `+` and choose `x64-Release`. Then select the new "x64-Release" from the Solution Configuration dropdown. Finally, right-click on `CMakeLists.txt` again and choose "Install".

#### Building cesium-native with Visual Studio Code

Basically just open the `C:\Dev\cesium-unreal-samples\Plugins\cesium-unreal\extern` folder in Visual Studio Code. More detailed instructions coming soon...

#### Building cesium-native with a CMake command-line

- Change into the `C:\Dev\cesium-unreal-samples\Plugins\cesium-unreal\extern` directory. 
  **Note**: This must be the `extern` directory, and *not* the `cesium-native` subdirectory!
- To generate project files for Visual Studio 2019, type the following command:

      cmake -B build -S . -G "Visual Studio 16 2019"

- To generate proejct files for Visual Studio 2017, type the following command:

      cmake -B build -S . -G "Visual Studio 15 2017 Win64"

This will generate the project file called `cesium-unreal-extern.sln` in the directory `C:\Dev\cesium-unreal-samples\Plugins\cesium-unreal\extern\build`. 

#### Building cesium-native with the CMake GUI

- Start `cmake-gui`
- In the "Where is the source code" text field, enter
  `C:\Dev\cesium-unreal-samples\Plugins\cesium-unreal\extern`
  **Note**: This must be the `extern` directory, and *not* the `cesium-native` subdirectory!
- In the "Where to build the binaries" text field, enter
  `C:\Dev\cesium-unreal-samples\Plugins\cesium-unreal\extern\build`
- Press "Configure" (and confirm the creation of the directory and the default generator for the project)
- Press "Generate"

This will generate the project file called `cesium-unreal-extern.sln` in the directory `C:\Dev\cesium-unreal-samples\Plugins\cesium-unreal\extern\build`. 

#### Creating the project files for the main project and the plugin

The project files for the project, *including* the actual *Cesium for Unreal* plugin, can be created with the Unreal Engine: 

- Using the Windows Explorer, browse into the `C:\Dev\cesium-unreal-samples` directory
- Right-click on the `CesiumForUnrealSamples.uproject` file
- Select "Generate Visual Studio project files"

This will generate the `CesiumForUnrealSamples.sln` file that can be opened with Visual Studio.


## Development process

The following description focusses on *development*, which usually means that "Debug" builds are used. 

### Developing in `cesium-native` 

The main development for `cesium-native` can take place in Visual Studio. Just open the `C:\Dev\cesium-unreal-samples\Plugins\cesium-unreal\extern\build\cesium-unreal-extern.sln` file with Visual Studio. 

After doing any modifications in `cesium-native`, the resulting binaries and header files have to be **installed**, so that they are recognized by the main project and the plugin. This can either be done on the console, or directly from Visual Studio:

On the console, change into the `C:\Dev\cesium-unreal-samples\Plugins\cesium-unreal\extern\build` directory, and call

    cmake --build build --config Debug --target install

Alternatively, in Visual Studio:

- Select the "Solution Configuration" in the toolbar (usually "Debug"),
- In the "Solution Explorer", under the "CMakePredefinedTargets" section, right click on "INSTALL" and select "Build"


### Developing for the Cesium for Unreal plugin itself

Open the `C:\Develop\CesiumUnreal\Dev\cesium-unreal-samples\CesiumForUnrealSamples.sln` file with Visual Studio. The Unreal Editor can directly be started from Visual Studio

- Select the "Solution Configuration" in the toolbar (during development, this will usually be "DebugGame Editor"). 
- Press F5 (or select "Debug->Start Debugging" from the menu) to start in debug mode

Starting in debug mode makes it possible to handle breakpoints, inspect variables with the debugger, or receive stack trace information in the case of crashes. For pure feature tests, it is also possible to press CTRL-F5 (or select "Debug->Start Without Debugging" from the menu) to start.


### Development notes

When developing both in `cesium-native` *and* in the actual plugin, it is convenient to have two instances of Visual Studio running

- In the `cesium-native` instance: After doing changes in the `cesium-native` part, right-click the "INSTALL" target as described above. This will install the updated library for the plugin
- In the `cesium-unreal-samples` instance: **After** installing an updated library for `cesium-native`, the build process should recognize that the library was updated. Compiling the plugin (or just starting the Unreal Editor from Visual Studio) should therefore cause the updated library to be integrated in the plugin.

If modifications in the `cesium-native` part seem to have no effect on the plugin, deleting the `C:\Develop\CesiumUnreal\Dev\cesium-unreal-samples\Binaries` and `C:\Dev\cesium-unreal-samples\Plugins\cesium-unreal\Binaries` directories will force a fresh rebuild, which can help to solve this problem.
