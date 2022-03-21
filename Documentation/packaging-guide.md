
# Packaging

Our Travis CI automatically creates a plugin package on every commit to the Cesium for Unreal repo. But if you want to do it locally, there are a few things to keep in mind:

* Unreal Engine always uses Visual Studio 2017 to package a plugin. So if you only have Visual Studio 2019 installed, the package process will fail.
* Similarly, if you built cesium-native with Visual Studio 2019, you will get linker errors when you try to build Cesium for Unreal with Visual Studio 2017. So, be sure to build cesium-native with Visual Studio 2017 if you need to package.

# Packaging Cesium for Unreal Plugin

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
    * Windows+Android Example (ensure you have completed the [Android-specific cross-compilation steps](https://github.com/CesiumGS/cesium-unreal/blob/main/Documentation/developer-setup-windows.md#cmake-command-line-for-android) first):
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


