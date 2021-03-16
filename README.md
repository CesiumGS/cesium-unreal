[![Cesium for Unreal Logo](Content/CESIUM-4-UNREAL-LOGOS_RGB_CESIUM-4-UNREAL-WhiteBGH.jpg)](https://www.unrealengine.com/marketplace/en-US/87b0d05800a545d49bf858ef3458c4f7)

The Cesium for Unreal plugin unlocks the entire 3D geospatial ecosystem in Unreal Engine. By combining a high-accuracy full-scale WGS84 globe, open APIs and open standards for spatial indexing like 3D Tiles, and cloud-based real-world content with the power of Unreal Engine, enables a new era of geospatial applications and workflows.

[Cesium for Unreal Homepage](https://cesium.com/cesium-for-unreal)

### :rocket: Get Started

**[Download Cesium for Unreal from Unreal Engine Marketplace](https://www.unrealengine.com/marketplace/en-US/87b0d05800a545d49bf858ef3458c4f7)** (Available March 30, 2021)

Have question? Ask them on the [community forum](https://community.cesium.com).

TODO: Unreal will create an Unreal Engine Forums post for the plugin. Include that here as well.

### :clap: Featured Demos

<p>
<a href="https://github.com/CesiumGS/cesium-unreal-demo"><img src="https://cesium.com/images/cesium-for-unreal/melbourne.jpg" width="48%" /></a>&nbsp;
<a href="https://www.unrealengine.com/en-US/industry/project-anywhere"><img src="https://cesium.com/blog/images/2020/11-30/Project-Anywhere-3.jpg" width="48%" /></a>&nbsp;
<br/>
<br/>
</p>

### :house_with_garden: Cesium for Unreal and the 3D Geospatial Ecosystem

Cesium for Unreal can stream real-world 3D content such as high-resolution photogrammetry, terrain, imagery, and 3D Buildings from the commercial [Cesium ion](https://cesium.com/cesium-ion) platform and other content sources. The plugin will include Cesium ion integration for instant access to global high-resolution 3D content ready for runtime streaming. Cesium ion users can also leverage cloud-based 3D tiling pipelines to create end-to-end workflows to transform massive heterogenous content into semantically-rich 3D Tiles, ready for streaming to Unreal Engine.

Cesium for Unreal will support cloud and private network content and services based on open standards and APIs. You are free to use any combination of content sources with Cesium for Unreal that you please.

[![Cesium for Unreal Ecosystem Architecture](https://cesium.com/images/graphics/unreal-pipeline.png)](https://cesium.com/cesium-for-unreal)

Using Cesium ion helps support Cesium for Unreal development. :heart:

### :card_index: Roadmap

TODO: Publish roadmap.

### :green_book: License

[Apache 2.0](http://www.apache.org/licenses/LICENSE-2.0.html). Cesium for Unreal is free for both commercial and non-commercial use.

### :computer: Using with Unreal Engine

#### :hammer_and_wrench: Compiling Cesium for Unreal

The following steps detail how to build the plugin and use it as part of your projects. You can also compile Cesium for Unreal as part of the [`cesium-unreal-demo`](https://github.com/CesiumGS/cesium-unreal-demo.git).

Cesium for Unreal depends on Cesium's high-precision geospatial C++ library - [Cesium Native](https://github.com/CesiumGS/cesium-native), which is included as a submodule.

1. Clone the repository using `git clone --recursive git@github.com:CesiumGS/cesium-unreal.git`.
2. From the `cesium-unreal/extern` directory, run the following commands to build `cesium-native`.

    ```cmd
    cmake -B build -S . -G "Visual Studio 15 2017 Win64" # Optionally use "Visual Studio 16 2019"
    cmake --build build --config Debug --target install # Optional, but recommended for debugging
    cmake --build build --config Release --target install # Optionally compile with --config RelWithDebInfo or MinSizeRel.
    ```

3. Point your Unreal Engine Project to the `CesiumForUnreal.uplugin` file to load the plugin into Unreal Engine.

#### :package: Packaging the Plugin

1. Clone [`cesium-unreal-demo`](https://github.com/CesiumGS/cesium-unreal-demo.git) and follow the build steps. Compile `cesium-native` in Release config with Visual Studio 2017.
2. In Unreal Engine, open `Edit->Plugins`. Search for `Cesium for Unreal`.
3. Click `Package` and select the output directory.
4. Wait for the compilation to finish.
5. Open the output directory and zip `CesiumForUnreal` directory.
