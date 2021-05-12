Detailed instructions for setting up a Cesium for Unreal development environment on Windows. Please see the [Developer Setup](developer-setup.md) page for an overview of the process.

# Prerequisities

- Install CMake (version 3.15 or newer) from https://cmake.org/install/
- Install Visual Studio 2017 v15.6+ or Visual Studio 2019 v16.5+.
- Install the Unreal Engine (version 4.26 or newer) from https://www.unrealengine.com/en-US/download

# Cloning the git repos

The following illustrates the recommended directory layout for developers:

- `C:\Dev` - Your own root directory for development. Keep it short!
- `C:\Dev\cesium-unreal-samples` - The directory for the Unreal project that will use the plugin.
- `C:\Dev\cesium-unreal-samples\Plugins\cesium-unreal` - The directory for the actual *Cesium for Unreal* plugin.
- `C:\Dev\cesium-unreal-samples\Plugins\cesium-unreal\extern\cesium-native` - The directory for the base libraries project.

You may use any directory for the project, but the directory for the actual *Cesium for Unreal* plugin **MUST** be in a subdirectory `Plugins/cesium-unreal` of the project directory. This way, Unreal will automatically find the Plugin when running the project, and pick up any changes that have been made to the plugin.

> On Windows, it is important that the top-level project directory have a short pathname. Otherwise, you may run into mysterious errors caused by the Windows [maximum path length limitation](https://docs.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation?tabs=cmd).

This can be set up with the following sequence of commands, on the console, starting in the `C:\Dev` directory:

    git clone https://github.com/CesiumGS/cesium-unreal-samples.git
    cd cesium-unreal-samples
    mkdir Plugins
    cd Plugins
    git clone --recursive https://github.com/CesiumGS/cesium-unreal.git

> Note: The last line will also check out the `cesium-native` submodule and its dependencies. If you forget the `--recursive` option, you will see many compiler errors later in this process. If this happens to you, run the following in the `Plugins\cesium-unreal` directory to update the submodules in the existing clone:

     git submodule update --init --recursive

# Building cesium-native

The cesium-native libraries and their dependencies use CMake and must be built separately from Cesium for Unreal. There are a number of ways to do this, depending on your preferred environment:

* [Visual Studio 2019](#Visual-Studio-2019)
* [Visual Studio Code](#Visual-Studio-Code)
* [CMake command-line](#CMake-command-line)
* [CMake GUI](#CMake-GUI)

The version of CMake included with Visual Studio 2017 is too old to build cesium-native, so to build with Visual Studio 2017, follow the CMake command-line or CMake GUI instructions.

## Visual Studio 2019

Visual Studio 2019 has built-in support for CMake as long as the "C++ CMake tools for Windows" option was selected at install time. It is included by default in the "Desktop Development with C++" workload.

To begin, launch Visual Studio 2019 and "Open a local folder". Select the `C:\Dev\cesium-unreal-samples\Plugins\cesium-unreal\extern`. Be sure to select the `extern` directory, *not* the `cesium-native` subdirectory.

Then, in the "Solution Explorer - Folder View", right-click on the root `CMakeLists.txt` and select "Install". This will compile a debug build of cesium-native and "install" it to the place in the project that Cesium for Unreal expects to find it: `c:\Dev\cesium-unreal-samples\Plugins\cesium-unreal\Source\ThirdParty`.

To also build a "Release" build of cesium-native, right click on `CMakeLists.txt` and select "CMake Settings for cesium-unreal-extern". Add a new configuration by clicking the `+` and choose `x64-Release`. Then select the new "x64-Release" from the Solution Configuration dropdown. Finally, right-click on `CMakeLists.txt` again and choose "Install".

## Visual Studio Code

Basically just open the `C:\Dev\cesium-unreal-samples\Plugins\cesium-unreal\extern` folder in Visual Studio Code and invoke the "install" target. More detailed instructions coming soon...

## CMake command-line

First, configure the CMake project in the `C:\Dev\cesium-unreal-samples\Plugins\cesium-unreal\extern` directory by following the instructions below.
**Note**: The following steps must be done in the `extern` directory, and *not* the `cesium-native` subdirectory!

To configure for Visual Studio 2019, open "x64 Native Tools Command Prompt for VS 2019" and execute the following command:

      cmake -B build -S . -G "Visual Studio 16 2019" -A x64

To use Visual Studio 2017 instead, open "x64 Native Tools Command Prompt for VS 2017" and execute the following command:

      cmake -B build -S . -G "Visual Studio 15 2017 Win64"

With either compiler, the commands above will generate the project file called `cesium-unreal-extern.sln` in the directory `C:\Dev\cesium-unreal-samples\Plugins\cesium-unreal\extern\build`. You can open this solution file in the Visual Studio IDE and compile as normal. To install cesium-native to the project - which is required for use with Cesium for Unreal - open the `CMakePredefinedTargets` folder in the Solution Explorer, right-click on `INSTALL`, and choose Build. Use the Solution Configuration dropdown to change between the Debug and Release configurations.

You can also build the Release version entirely from the command-line:

      cmake --build build --config Release --target install

Or the debug version:

      cmake --build build --config Debug --target install

## CMake GUI

- Start `cmake-gui`
- In the "Where is the source code" text field, enter
  `C:\Dev\cesium-unreal-samples\Plugins\cesium-unreal\extern`
  **Note**: This must be the `extern` directory, and *not* the `cesium-native` subdirectory!
- In the "Where to build the binaries" text field, enter
  `C:\Dev\cesium-unreal-samples\Plugins\cesium-unreal\extern\build`
- Press "Configure" (and confirm the creation of the directory and the default generator for the project)
- Press "Generate"

This will generate the project file called `cesium-unreal-extern.sln` in the directory `C:\Dev\cesium-unreal-samples\Plugins\cesium-unreal\extern\build`. You can open this solution file in the Visual Studio IDE and compile as normal. To install cesium-native to the project - which is required for use with Cesium for Unreal - open the `CMakePredefinedTargets` folder in the Solution Explorer, right-click on `INSTALL`, and choose Build. Use the Solution Configuration dropdown to change between the Debug and Release configurations.

# Creating the project files for the Unreal Engine game/project and the Cesium for Unreal plugin

The project files for the project, *including* the actual *Cesium for Unreal* plugin, can be created with the Unreal Engine: 

- Using the Windows Explorer, browse into the `C:\Dev\cesium-unreal-samples` directory
- Right-click on the `CesiumForUnrealSamples.uproject` file
- Select "Generate Visual Studio project files"

This will generate the `CesiumForUnrealSamples.sln` file that can be opened, compiled, and debugger with Visual Studio. Be sure to switch the "Solution Platform" to "Win64".

If the above yields an error like this:

      This project does not have any source code. You need to add C++
      source files to the project from the Editor before you can generate
      project files.

It indicates that your project is Blueprints-only. See [Convert a Blueprints Project to C++](convert-blueprints-project-to-cpp.md) to enable the use of C++ code in your project so that it can contain an embedded C++ plugin like Cesium for Unreal. After you complete that process, you should now be able to "Generate Visual Studio project files" successfully.

If you have both Visual Studio 2017 and Visual Studio 2019 installed, the Visual Studio project files generated above may build with VS2017 even if they're opened with VS2019. This will still be true even if you allow VS2019 to upgrade the project files to the VS2019 toolchain. That's because the project files simply invoke the Unreal Build Tool, which plays by its own rules. This is generally not a problem, except if you used used Visual Studio 2019 to build cesium-native. In that case, you will get linker errors when you try to compile Cesium for Unreal.

To switch the Unreal Build Tool to use VS2019 instead, launch the Unreal Editor and open a project that does _not_ use Cesium for Unreal. Go to `Edit` -> `Editor Preferences`. Go to the `General` -> `Source Code` section and change `Source Code Editor` to `Visual Studio 2019`. Click "Set as Default" so that this change applies to all projects that don't override it.

# Solution Configurations

During development, you will typically use the "DebugGame Editor" or "Development Editor" Solution Configuration in the Visual Studio solution created above. "DebugGame Editor" is easier to debug, but "Development Editor" will be a bit faster. In either case, but sure that "Win64" is selected as the Solution Platform.

When you build "DebugGame Editor", the build process will _first_ look for an installed Debug build of cesium-native. If it finds one, it will use it. If not, it will try to use an installed Release build of cesium-native instead. The "Development Editor" configuration, on the other hand, will _always_ use an installed Release build of cesium-native, and will fail to build if one does not exist.

So, when you make changes to cesium-native code, make sure you are building _and installing_ the correct configuration of cesium-native for the Cesium for Unreal configuration that you're using:

* `Development Editor` -> `Release`
* `DebugGame Editor` -> `Debug`

See the sections above to learn how to build the Debug and Release configurations of cesium-native in your preferred environment.

While actively making changes to cesium-native, it is usually convenient to have two copies of Visual Studio open: one for cesium-native, and one for Cesium for Unreal.

# Debugging

In the Cesium for Unreal solution in Visual Studio, press F5 (or select "Debug->Start Debugging" from the menu) to start in debug mode.

Starting in debug mode makes it possible to set breakpoints, inspect variables with the debugger, or receive stack trace information in the case of crashes. For pure feature tests, it is also possible to press CTRL-F5 (or select "Debug->Start Without Debugging" from the menu) to start.

When using the "DebugGame Editor" solution configuration and the Debug configuration of cesium-native, it is also possible to debug through cesium-native code. First, make sure "Just my Code" is disabled:

- Go to `Tools` -> `Options`.
- Navigate to `Debugging` -> `General`.
- Verify that `Enable Just My Code` is unchecked.

Once that is done, it is possible to set breakpoints in Cesium for Unreal plugin code and step from there straight into cesium-native code. It's also possible to set breakpoints in cesium-native code directly, but it can be a bit of a hassle to navigate to those files because they aren't in the Cesium for Unreal solution. There are two ways to open cesium-native files from the Cesium for Unreal solution for debugging purposes:

- Switch the Solution Explorer to Folder View, turn on "Show All Files", and then navigate to the cesium-native code in `Plugins\cesium-unreal\extern\cesium-native`. Getting back to the default Solution Explorer view can be a pain. The trick is to double-click the .sln file in Folder View.
- Find the file you want to debug in another copy of Visual Studio which is open on the cesium-native solution. Then, right-click on the file's tab and choose "Copy Full Path". Go back to the Cesium for Unreal solution, go to `File` -> `Open` -> `File`, and paste in the copied file path.

If you find the debugger refuses to step into cesium-native code, check that you're using the "DebugGame Editor" configuration of Cesium for Unreal and the Debug configuration of cesium-native, and that you've built and installed cesium-native.

# Packaging

Our Travis CI automatically creates a plugin package on every commit to the Cesium for Unreal repo. But if you want to do it locally, there are a few things to keep in mind:

* Unreal Engine always uses Visual Studio 2017 to package a plugin. So if you only have Visual Studio 2019 installed, the package process will fail.
* Similarly, if you built cesium-native with Visual Studio 2019, you will get linker errors when you try to build Cesium for Unreal with Visual Studio 2017. So, be sure to build cesium-native with Visual Studio 2017 if you need to package.
