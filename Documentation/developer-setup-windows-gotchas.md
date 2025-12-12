# Developer Setup for Windows - Gotchas {#developer-setup-windows-gotchas}

Compiling Cesium for Unreal requires that the vcpkg-based third-party dependencies, the cesium-native code, and the Cesium for Unreal code itself are compiled using the same (or compatible) versions of the MSVC compiler. This is remarkably hard to achieve! This guide will hopefully help you get there.

Some important facts to understand before continuing:

1. "Visual Studio 2022" is not a compiler version. It's an IDE brand name, and may imply any number of compiler versions.
2. Updates to Visual Studio 2022 often come with new versions of the MSVC compiler. For Visual Studio 2022, the MSVC compilers have version numbers like 14.x. Sometimes the compiler version is referred to as a "toolchain" version.
3. Microsoft _does_ allow us to link together .lib files built with different versions of the MSVC compiler. However, the compiler version used to _link_ must be the same or newer than the _newest_ compiler that built any of the .libs (or any object files they contain).
4. You can install any number of toolchain versions simultaneously. Go to "Add / Remove Programs" and Modify "Visual Studio Professional 2022". Then click the "Individual Components" tab and scroll down to "Compilers, build tools, and runtimes". Tick the box next to the "MSVC v143 - VS2022 C++ x64/x86 build tools" version that you want to install.

## Cesium for Unreal

The Cesium for Unreal plugin code includes and links with a great deal of Unreal Engine code. It is built by the Unreal Build Tool, even when you compile from within Visual Studio.

The MSVC compiler version that Epic used to build a release version of Unreal Engine can be found in that version's release notes. For example, the [release notes for Unreal Engine 5.5](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-5-5-release-notes?application_version=5.5) include this information:

> * IDE Version the Build farm compiles against
>   * Visual Studio 2022 17.8 14.38.33130 toolchain and Windows 10 SDK (10.0.22621.0)

For maximum compatibility, released versions of Cesium for Unreal should be built with _exactly_ this version, v14.38.33130, and this is what we do on CI.

However, when compiling for your own development purposes, you can use this version or any compatible newer one. In general, newer compilers work just fine, but not always. Older Unreal Engine versions are not tested against MSVC toolchain versions that came out after they're released (for obvious reasons), so sometimes, these new versions can't compile the older Unreal Engine code.

In addition, Unreal Engine versions sometimes specify a _preferred_ MSVC version that the Unreal Build Tool will automatically use if it's installed. This is found in `PreferredVisualCppVersions` in `Engine/Config/Windows/Windows_SDK.json` in the UE installation. For UE 5.5 and 5.6 it is 14.38, and for 5.7 it is 14.44.

It's important to be aware of which MSVC version the Unreal Build Tool is using on your system so that you can make sure cesium-native and vcpkg dependencies (see below) use that same version _or older_.

If you need to, you can explicitly tell the Unreal Build Tool to use a specific toolchain version. To do that, open <!--! \cond DOXYGEN_EXCLUDE !-->`%AppData%\Unreal Engine\UnrealBuildTool\BuildConfiguration.xml`<!--! \endcond --><!--! `%%AppData%\Unreal Engine\UnrealBuildTool\BuildConfiguration.xml` --> and add this to it:

```xml
<?xml version="1.0" encoding="utf-8" ?>
<Configuration xmlns="https://www.unrealengine.com/BuildConfiguration">
	<WindowsPlatform>
		<CompilerVersion>14.38.33130</CompilerVersion>
	</WindowsPlatform>
</Configuration>
```

> [!note]
> The environment variable <!--! \cond DOXYGEN_EXCLUDE !-->`%AppData%`<!--! \endcond --><!--! `%%AppData%` --> resolves to something like `C:\Users\UserName\AppData\Roaming`. In PowerShell, use `$env:AppData` instead.

With that, all builds invoked by Unreal Build Tool should use the chosen compiler version. You should see a message near the start of the build log confirming this:

> Using Visual Studio 2022 14.38.33144 toolchain (C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC\14.38.33130) and Windows 10.0.22621.0 SDK (C:\Program Files (x86)\Windows Kits\10).

To see what complete version numbers you have installed, look in `C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC` or similar on your system.

## vcpkg

vcpkg has hard-coded logic to choose the very latest version of the compiler that you have installed. It completely ignores all the usual ways that different compiler versions can be selected, such as setting the defaults file (`"C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.props"`), setting environment variables or running the `vcvarsall` script.

As explained above, if you're building for Unreal Engine 5.5 and 5.6, Unreal Build Tool will choose v14.38 of the compiler if it's installed, and 5.7 will use v14.44. This will lead to linker errors if you have a newer version installed, because vcpkg chooses the latest compiler version, and you'll be linking with an older compiler than was used to compile. 

The easiest way to select a compiler for vcpkg to use is to set the `VCPKG_PLATFORM_TOOLSET_VERSION` environment variable. Despite the name, this is not built-in vcpkg functionality, but is instead something that our `x64-windows-unreal.cmake` triplet file looks for explicitly. To set it, run the following in a PowerShell window:

```powershell
$env:VCPKG_PLATFORM_TOOLSET_VERSION = "14.38"
```

Or in a CMD prompt, run:

```bat
set VCPKG_PLATFORM_TOOLSET_VERSION=14.38
```

This environment variable must be set before running CMake to configure the `cesium-native` project.

Alternatively, you can explicitly specify the toolset version in the vcpkg triplet file. To build with 14.38, edit the `extern/vcpkg-overlays/triplets/x64-windows-unreal.cmake` file in the `cesium-unreal` repo and add this line to the end of it:

```
set(VCPKG_PLATFORM_TOOLSET_VERSION "14.38")
```

> [!warning]
> Be careful not to commit this change if you edit the triplet file directly! Using the environment variable is safer.

vcpkg won't always automatically rebuild everything it needs to rebuild after making this change. To force a rebuild, delete the following:

* The entire `.ezvcpkg` directory. It's usually found at `c:\.ezvcpkg` or `%userprofile%\.ezvcpkg`. If you're not sure, look for where ezvcpkg prints `local dir: d:/.ezvcpkg/dbe35ceb30c688bf72e952ab23778e009a578f18` or similar during the cesium-native CMake configure.
* The vcpkg [binary cache](https://learn.microsoft.com/en-us/vcpkg/users/binarycaching) directory. It's usually found at `%LOCALAPPDATA%\vcpkg\archives` (or `$env:LOCALAPPDATA\vcpkg\archives` in PowerShell).

## cesium-native

cesium-native chooses compilers in the normal CMake way. That means that, by default, it will use the compiler version that you installed most recently (which is _not_ necessarily the latest). So if you installed 14.38 following the instructions above, the cesium-native build should automatically start using it. You may need to delete your `build` directory.

To specify a particular compiler, you can use `vcvarsall.bat`:

```
"C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64 -vcvars_ver=14.38
```

Note that you must do this from a CMD prompt, _not_ from PowerShell. But you can run PowerShell afterward by invoking `pwsh` or `powershell`.

You can also specify the compiler version on the CMake command-line:

```
cmake -B build -S . -T "version=14.38"
```

A common scenario is that you've followed the instructions in this document and installed v14.38, and your Cesium for Unreal build is working fine. Then, you try to compile some _other_ CMake-based project that should be using the newer compiler, and it doesn't work anymore. The other project's build is unexpectedly using the v14.38 compiler. It makes sense that 14.38 would be the default, because that's the one you installed most recently. But even following either one or both of the suggestions above doesn't help. CMake continues to insist on using 14.38. What's the deal?

The solution here is to explicitly install the version of the build tools that you're trying to use. In "Add / Remove Programs" -> Modify "Visual Studio Professional 2022" -> "Invididual Components" tabs, you'll probably see that the box next to the "MSVC v143 - VS2022 C++ x64/x86 build tools (Latest)" is ticked. But the specific version that actually is the latest (14.44 as of this writing) is _not_ ticked. If you install that one, you will now be able to explicitly select it in CMake builds using either of the techniques above. It is very frustating that this kind of thing is required.

## Visual Studio Code

You may have different projects that need to use different compiler versions. If you use Visual Studio Code, the easiest way to deal with this is to set up custom "kits" for the different versions.

Open the command palette (Ctrl-Shift-P) and choose `CMake: Edit User-Local CMake Kits`. Make two copies of the JSON object for the existing `amd64` kit. It will have a name similar to `Visual Studio Professional 2022 Release - amd64`. Name the two copies for the compiler version you want each to use, such as `14.38 - Visual Studio Professional 2022 Release - amd64`. Then, do the following:

* Add the `version=14.38` or similar to the `preferredGenerator.toolset` property.
* Create an entry for the `VCToolsVersion` environment variable with the full three-part version. You can find the three-part version for your installed toolsets by looking in `C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC` or similar.

It should look like this (though your names and `visualStudio` properties may be different):

```json
[
  {
    "name": "14.38 - Visual Studio Professional 2022 Release - amd64",
    "visualStudio": "7d903395",
    "visualStudioArchitecture": "x64",
    "isTrusted": true,
    "preferredGenerator": {
      "name": "Visual Studio 17 2022",
      "platform": "x64",
      "toolset": "host=x64,version=14.38"
    },
    "environmentVariables": {
      "VCToolsVersion": "14.38.33130",
      "VCPKG_PLATFORM_TOOLSET_VERSION": "14.38"
    }
  },
  {
    "name": "14.44 - Visual Studio Professional 2022 Release - amd64",
    "visualStudio": "7d903395",
    "visualStudioArchitecture": "x64",
    "isTrusted": true,
    "preferredGenerator": {
      "name": "Visual Studio 17 2022",
      "platform": "x64",
      "toolset": "host=x64,version=14.44"
    },
    "environmentVariables": {
      "VCToolsVersion": "14.44.35207",
      "VCPKG_PLATFORM_TOOLSET_VERSION": "14.44"
    }
  },
  ...
]
```

You can now choose your compiler by invoking `CMake: Select a kit` from the command palette.

If this doesn't work (your build uses the wrong compiler), check that you've explicitly installed both compiler versions, as described at the end of the [cesium-native](#cesium-native) section above.
