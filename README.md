[![Cesium for Unreal Logo](Content/Cesium-for-Unreal-Logo-WhiteBGH.jpg)](https://www.unrealengine.com/marketplace/en-US/87b0d05800a545d49bf858ef3458c4f7)

Cesium for Unreal brings the 3D geospatial ecosystem to Unreal Engine. By combining a high-accuracy full-scale WGS84 globe, open APIs and open standards for spatial indexing such as 3D Tiles, and cloud-based real-world content from [Cesium ion](https://cesium.com/cesium-ion) with Unreal Engine, this project enables a new era of 3D geospatial software.

[Cesium for Unreal Homepage](https://cesium.com/cesium-for-unreal)

### :rocket: Get Started

**[Download Cesium for Unreal from Unreal Engine Marketplace](https://www.unrealengine.com/marketplace/en-US/87b0d05800a545d49bf858ef3458c4f7)**

Have questions? Ask them on the [community forum](https://community.cesium.com).

### :clap: Featured Demos

<p>
<a href="https://github.com/CesiumGS/cesium-unreal-demo"><img src="https://cesium.com/images/cesium-for-unreal/melbourne.jpg" width="48%" /></a>&nbsp;
<a href="https://www.unrealengine.com/en-US/industry/project-anywhere"><img src="https://cesium.com/blog/images/2020/11-30/Project-Anywhere-3.jpg" width="48%" /></a>&nbsp;
<br/>
<br/>
</p>

### :house_with_garden: Cesium for Unreal and the 3D Geospatial Ecosystem

Cesium for Unreal can stream real-world 3D content such as high-resolution photogrammetry, terrain, imagery, and 3D buildings from [Cesium ion](https://cesium.com/cesium-ion) and other sources, available as optional commercial subscriptions. The plugin includes Cesium ion integration for instant access to global high-resolution 3D content ready for runtime streaming. Cesium ion users can also leverage cloud-based 3D tiling pipelines to create end-to-end workflows to transform massive heterogenous content into semantically-rich 3D Tiles, ready for streaming to Unreal Engine.

Cesium for Unreal supports cloud and private network content and services based on open standards and APIs. You are free to use any combination of supported content sources, standards, APIs with Cesium for Unreal.

[![Cesium for Unreal Ecosystem Architecture](https://cesium.com/images/graphics/unreal-pipeline.png)](https://cesium.com/cesium-for-unreal)

Using Cesium ion helps support Cesium for Unreal development. :heart:

### :chains: Unreal Engine Integration

Cesium for Unreal is tightly integrated with Unreal Engine making it possible to visualize and interact with real-world content in editor and at runtime. The plugin also has support for Unreal Engine physics, collisions, character interaction, and landscaping tools. Leverage decades worth of cutting-edge advancements in Unreal Engine and geospatial to create cohesive, interactive, and realistic simulations and applications with Cesium for Unreal.

### :green_book: License

[Apache 2.0](http://www.apache.org/licenses/LICENSE-2.0.html). Cesium for Unreal is free for both commercial and non-commercial use.

### :computer: Developing with Unreal Engine

#### :hammer_and_wrench: Compiling Cesium for Unreal

The following steps detail how to build the plugin and use it as part of your projects. You can also compile Cesium for Unreal as part of the [`cesium-unreal-samples`](https://github.com/CesiumGS/cesium-unreal-samples.git).

Cesium for Unreal depends on Cesium's high-precision geospatial C++ library - [Cesium Native](https://github.com/CesiumGS/cesium-native), which is included as a submodule.

1. Clone the repository using `git clone --recursive git@github.com:CesiumGS/cesium-unreal.git`.
2. From the `cesium-unreal/extern` directory, run the following commands to build `cesium-native`.

    ```cmd
    cmake -B build -S . -G "Visual Studio 15 2017 Win64" # Optionally use "Visual Studio 16 2019"
    cmake --build build --config Debug --target install # Optional, recommended for debugging
    cmake --build build --config Release --target install # Can optionally compile with --config RelWithDebInfo or MinSizeRel.
    ```

3. Point your Unreal Engine Project to the `CesiumForUnreal.uplugin` file to load the plugin into Unreal Engine.

#### :package: Accessing Packaged Plugin

The easiest way to access Cesium for Unreal is by downloading it from the [Unreal Engine Marketplace](https://www.unrealengine.com/marketplace/en-US/87b0d05800a545d49bf858ef3458c4f7).

If you would like to access development versions of the plugin, Cesium for Unreal has Travis CI integration that prepares packages with each CI build. To access these packages, click the ✔️ icon on the GitHub branch or commit and click the `Details` next to `plugin-package`. You can extract the downloaded plugin package into your Unreal project's `Plugins` directory.
