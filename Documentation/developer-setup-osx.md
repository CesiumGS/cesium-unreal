# Developer Setup for macOS {#developer-setup-osx}

Detailed instructions for setting up a Cesium for Unreal development environment on macOS. Please see the [Developer Setup](developer-setup.md) page for an overview of the process.
<!--! [TOC] -->

# Prerequisities

- Install CMake (version 3.15 or newer) from https://cmake.org/install/
- Install Xcode 14.2+ from https://developer.apple.com/xcode/resources/
- For best JPEG-decoding performance, you must have [nasm](https://www.nasm.us/) installed so that CMake can find it. Everything will work fine without it, just slower.
- Install the minimum supported version of Unreal Engine (version 5.3 as of this writing) from https://www.unrealengine.com/en-US/download

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
> [!note]
> The last line will also check out the `cesium-native` submodule and its dependencies. If you forget the `--recursive` option, you will see many compiler errors later in this process. If this happens to you, run the following in the `Plugins\cesium-unreal` directory to update the submodules in the existing clone:
```
git submodule update --init --recursive
```

# Setting up Xcode

Unreal Engine 5.3 requires a version of Xcode _no later_ than Xcode 15.x. This means that Xcode 16, which is the earliest version supported on macOS 15.3 Sequoia, cannot be used to build a UE 5.3 project without some tricks. You will see an error like this when you attempt to generate project files:

> Exception while generating include data for UnrealEditor: Platform Mac is not a valid platform to build. Check that the SDK is installed properly.

We have a few options:

1. Use an earlier version of macOS
2. Use a later version of Unreal Engine
3. [Hack UnrealBuildTool to allow us to build for UE 5.3 using Xcode 16](#modify-unrealbuildtool-for-xcode-16)
4. [Hack Xcode 15.4 to run on macOS 15.2 Sequoia](#modify-xcode-for-sequoia)

## Modify UnrealBuildTool for Xcode 16

The source code for the UnrealBuildTool is installed with Unreal Engine, which makes it easy to modify it for our purposes. The file to edit in Unreal Engine 5.3 is here (or equivalent on your system):

```
/Users/Shared/Epic Games/UE_5.3/Engine/Source/Programs/UnrealBuildTool/Platform/Mac/ApplePlatformSDK.Versions.cs
```

Find a line in that file that looks like this:

```
MaxVersion = "15.9.9";
```

If you have Xcode 16, change it to:

```
MaxVersion = "16.9.9";
```

Now we need to build our modifications into a new binary, which we can do by running:

```
cd "/Users/Shared/Epic Games/UE_5.3/Engine/Source/Programs/UnrealBuildTool"
"/Users/Shared/Epic Games/UE_5.3/Engine/Binaries/ThirdParty/DotNet/6.0.302/mac-x64/dotnet" build UnrealBuildTool.csproj
```

> [!note]
> The path to the `dotnet` executable may be different in different Unreal Engine versions. You can also install a system `dotnet` if desired with `brew install dotnet`.

This will build UnrealBuildTool to the place all of Unreal's build scripts expect to find it. You should now be able to [generate project files](#building-cesium-for-unreal) successfully.

However, because this version of Unreal Engine has not been tested on this Xcode version, you may run into compiler errors, and these may be easy or difficult to fix. With Unreal Engine 5.3 and Xcode 16.2, you'll likely see errors like this:

> /Users/Shared/Epic Games/UE_5.3/Engine/Source/Runtime/RenderCore/Public/ShaderParameterStructDeclaration.h:22:3: encoding prefix 'u' on an unevaluated string literal has no effect and is incompatible with c++2c [-Werror,-Winvalid-unevaluated-string]

We can "fix" this by suppressing this warning. Open `/Users/Shared/Epic Games/UE_5.3/Engine/Source/Runtime/Core/Public/Apple/ApplePlatformCompilerPreSetup.h` and add a line to disable this warning:

```
#pragma clang diagnostic ignored "-Winvalid-unevaluated-string"
```

## Modify Xcode for Sequoia

It's possible to run Xcode 15.4 on macOS 15.3 Sequoia, and probably later versions:

* Download Xcode 15.4 from https://developer.apple.com/download/all/.
* Extract the download to your home directory:

```
cd
xip -x ./Downloads/Xcode_15.4.xip
mv Xcode.app Xcode_15.4.app
```

* Launch this version of Xcode:

```
./Xcode_15.4.app/Contents/MacOS/Xcode
```

* Xcode should launch and ask you to select or create a project. Choose `Xcode` on the menu at the top and then `Settings`. Click the `Locations` tab.
* Under `Command Line Tools` choose `Xcode_15 15.4`.

# Building cesium-native

The cesium-native libraries and their dependencies use CMake and must be built separately from Cesium for Unreal. Cesium for Unreal supports both Intel and Apple Silicon processors. In development, we usually just want to build for the host's architecture, which is done as follows:

```
cd ~/dev/cesium-unreal-samples/Plugins/cesium-unreal/extern
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target install --parallel 14
```

Or to build a Release version:

```
cd ~/dev/cesium-unreal-samples/Plugins/cesium-unreal/extern
cmake -B build -S . -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --target install --parallel 14
```

This will install the built libraries to one of these subdirectories of `~/dev/cesium-unreal-samples/Plugins/cesium-unreal/Source/ThirdParty/lib/`, depending on your configuration and processor architecture:

* `Darwin-arm64-Debug` - Debug configuration for Apple Silicon.
* `Darwin-arm64-Release` - Release or RelWithDebInfo configuration for Apple Silicon.
* `Darwin-x64-Debug` - Debug configuration for Intel processors.
* `Darwin-x64-Release` - Release or RelWithDebInfo configuration for Intel processors.

Cesium for Unreal expects to find the libraries in one of these locations, depending on its configuration:

* `Darwin-universal-Debug` - DebugGame configuration for both processors.
* `Darwin-universal-Release` - Development and Shipping configuration for both processors.

If we only care to run on the host's architecture, such as during development, we can create a symlink to enable the Cesium for Unreal build to find the libraries for the one processor:

```
cd ~/dev/cesium-unreal-samples/Plugins/cesium-unreal/Source/ThirdParty/lib
ln -s ./Darwin-arm64-Debug Darwin-universal-Debug
ln -s ./Darwin-arm64-Release Darwin-universal-Release
```

Or, we can build for the other processor:

```
cd ~/dev/cesium-unreal-samples/Plugins/cesium-unreal/extern
# Remove the symlink if it exists
rm ~/dev/cesium-unreal-samples/Plugins/cesium-unreal/Source/ThirdParty/lib/Darwin-universal-Release
# Build for Intel
cmake -B build-x64 -S . -DCMAKE_OSX_ARCHITECTURES=x86_64  -DCMAKE_SYSTEM_NAME=Darwin -DCMAKE_SYSTEM_PROCESSOR=x86_64 -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --target install --parallel 14
```

And then create universal libraries using lipo:

```
mkdir -p ~/dev/cesium-unreal-samples/Plugins/cesium-unreal/Source/ThirdParty/lib/Darwin-universal-Release
for f in ~/dev/cesium-unreal-samples/Plugins/cesium-unreal/Source/ThirdParty/lib/Darwin-x86_64-Release/*.a
do
  arm64f=~/dev/cesium-unreal-samples/Plugins/cesium-unreal/Source/ThirdParty/lib/Darwin-arm64-Release/$(basename -- $f)
  x64f=~/dev/cesium-unreal-samples/Plugins/cesium-unreal/Source/ThirdParty/lib/Darwin-x86_64-Release/$(basename -- $f)
  universalf=~/dev/cesium-unreal-samples/Plugins/cesium-unreal/Source/ThirdParty/lib/Darwin-universal-Release/$(basename -- $f)
  if diff $arm64f $x64f; then
    # arm64 and x64 files are identical, so just copy one to the universal directory.
    cp $arm64f $universalf
  else
    lipo -create -output $universalf $arm64f $x64f
  fi
done
```

## Building for iOS

**Note**: It is recommended that the build steps for CMake command-line for macOS (above) be completed first. Unreal Engine Editor will not launch without the host side binaries compiled as well.

Configure the CMake project in the `~/dev/cesium-unreal-samples/Plugins/cesium-unreal/extern` directory by following the instructions below. Use a different build directory than the one used for macOS as this will require compiling for a different architecture.
> [!note]
>  The following steps must be done in the `extern` directory, and _not_ the `cesium-native` subdirectory!

Execute the following commands to build and install a Release version of cesium-native:

```
cd ~/dev/cesium-unreal-samples/Plugins/cesium-unreal/extern
cmake -B build-ios -S . -GXcode -DCMAKE_TOOLCHAIN_FILE="unreal-ios-toolchain.cmake" -DCMAKE_BUILD_TYPE=Release
cmake --build build-ios --target install --config Release --parallel 14
```

You can also build and install the debug version by using `Debug` or `RelWithDebInfo` instead of `Release`.

# Building Cesium for Unreal

The Cesium for Unreal plugin must be built as part of a larger project, and that project must be a C++ project. The Cesium for Unreal Samples project is a Blueprint project, not C++, but it's easy to convert it to a C++ project by copying the `Source` directory from the documentation:

```
cd ~/dev/cesium-unreal-samples
cp -r ./Plugins/cesium-unreal/Documentation/Source .
```

Now we can generate Xcode project files for the Samples project and the plugin:

```
cd ~/dev/cesium-unreal-samples
"/Users/Shared/Epic Games/UE_5.3/Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh" -game -project="$PWD/CesiumForUnrealSamples.uproject"
```

You may see an error message like this:

> Your Mac is set to use CommandLineTools for its build tools (/Library/Developer/CommandLineTools). Unreal expects Xcode as the build tools. Please install Xcode if it's not already, then do one of the following:
>   - Run Xcode, go to Settings, and in the Locations tab, choose your Xcode in Command Line Tools dropdown.
>   - In Terminal, run 'sudo xcode-select -s /Applications/Xcode.app' (or an alternate location if you installed Xcode to a non-standard location)
> Either way, you will need to enter your Mac password.

In which case, do what it says.

If you see a message like this:

> Exception while generating include data for UnrealEditor: Platform Mac is not a valid platform to build. Check that the SDK is installed properly.

It probably means Unreal doesn't like your Xcode version. Be sure that Xcode is installed and that you have followed the [Xcode setup instructions](#setting-up-xcode).

If the project file generation succeeds, you should see a file named `CesiumForUnrealSamples (Mac).xcworkspace` in the same directory as your uproject. Double-click it to open Xcode.

In Xcode, on the Product -> Scheme menu, choose `devEditor`. If you want to build a debug configuration, go to Product -> Scheme -> Edit Scheme... and then change the "Build Configuration" to "DebugGame".

Build by choosing Product -> Build. Watch the progress in the "Report Navigator" which is the rightmost icon above the tree on the left.

You can launch the Unreal Engine Editor and the Samples project with Product -> Run.
