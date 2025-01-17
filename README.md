[![Cesium for Unreal Logo](Content/Cesium-for-Unreal-Logo-WhiteBGH.jpg)](https://cesium.com/unreal-marketplace?utm_source=cesium-unreal&utm_medium=github&utm_campaign=unreal)

_This branch targets Unreal Engine 5. There is also a branch targeting [Unreal Engine 4](../../tree/ue4-main)_

Cesium for Unreal brings the 3D geospatial ecosystem to Unreal Engine. By combining a high-accuracy full-scale WGS84 globe, open APIs and open standards for spatial indexing such as 3D Tiles, and cloud-based real-world content from [Cesium ion](https://cesium.com/cesium-ion) with Unreal Engine, this project enables a new era of 3D geospatial software.

[Cesium for Unreal Homepage](https://cesium.com/cesium-for-unreal?utm_source=github&utm_medium=github&utm_campaign=unreal)

### 🚀 Get Started

**[Download Cesium for Unreal from Unreal Engine Marketplace](https://cesium.com/unreal-marketplace?utm_source=cesium-unreal&utm_medium=github&utm_campaign=unreal)**

**[Follow the Quickstart](https://cesium.com/docs/tutorials/cesium-unreal-quickstart/)**

Have questions? Ask them on the [community forum](https://community.cesium.com).

### 👏 Featured Demos

<p>
<a href="https://github.com/CesiumGS/cesium-unreal-samples"><img src="https://images.prismic.io/cesium/bfa9f768-26eb-4a6f-a427-8e9cecbe16b1_melbourne.jpg" width="48%" /></a>&nbsp;
<a href="https://cesium.com/blog/2020/11/30/project-anywhere/"><img src="https://images.prismic.io/cesium/2020-11-30-Project-Anywhere-3.jpg" width="48%" /></a>&nbsp;
<br/>
<br/>
</p>

### 🏡 Cesium for Unreal and the 3D Geospatial Ecosystem

Cesium for Unreal streams real-world 3D content such as high-resolution photogrammetry, terrain, imagery, and 3D buildings from [Cesium ion](https://cesium.com/cesium-ion) and other sources, available as optional commercial subscriptions. The plugin includes Cesium ion integration for instant access to global high-resolution 3D content ready for runtime streaming. Cesium ion users can also leverage cloud-based 3D tiling pipelines to create end-to-end workflows to transform massive heterogenous content into semantically-rich 3D Tiles, ready for streaming to Unreal Engine.

Cesium for Unreal supports cloud and private network content and services based on open standards and APIs. You are free to use any combination of supported content sources, standards, APIs with Cesium for Unreal.

[![Cesium for Unreal Ecosystem Architecture](https://prismic-io.s3.amazonaws.com/cesium/b1505fbc-5769-4032-9233-364a4f52acf6_unreal-pipeline-ice-blue-background.png)](https://cesium.com/cesium-for-unreal?utm_source=cesium-unreal&utm_medium=github&utm_campaign=unreal)

Using Cesium ion helps support Cesium for Unreal development. ❤️

### ⛓️ Unreal Engine Integration

Cesium for Unreal is tightly integrated with Unreal Engine making it possible to visualize and interact with real-world content in editor and at runtime. The plugin also has support for Unreal Engine physics, collisions, character interaction, and landscaping tools. Leverage decades worth of cutting-edge advancements in Unreal Engine and geospatial to create cohesive, interactive, and realistic simulations and applications with Cesium for Unreal.

### 📗 License

[Apache 2.0](http://www.apache.org/licenses/LICENSE-2.0.html). Cesium for Unreal is free for both commercial and non-commercial use.

### 📦 Installing Cesium for Unreal

The easiest way to install Cesium for Unreal is by downloading the officially released version from the [Unreal Engine Marketplace](https://cesium.com/unreal-marketplace?utm_source=cesium-unreal&utm_medium=github&utm_campaign=unreal).

You can also find all releases on the [Releases](https://github.com/CesiumGS/cesium-unreal/releases) page. This is useful if you want an older version, or if you can't or don't want to use the Unreal Engine Marketplace. In particular, if you're using Linux, the Releases page is a better option. To install any of these releases:

1. If you previously installed the Cesium for Unreal plugin via the Unreal Engine Marketplace, uninstall it.
2. Extract the release ZIP to Unreal Engine's `Engine/Plugins/Marketplace` directory. For example, on Unreal Engine 5.3 on Windows, this is typically `C:\Program Files\Epic Games\UE_5.3\Engine\Plugins\Marketplace`. You may need to create the `Marketplace` directory yourself.
3. If you've done this correctly, you'll find a `CesiumForUnreal` sub-directory inside the `Marketplace` directory, and the plugin is ready to use.

You can also [use pre-release packages](Documentation/using-prerelease-packages.md).

### 💻 Developing with Unreal Engine

See the [Developer Setup Guide](Documentation/developer-setup.md) to learn how to set up a development environment for Cesium for Unreal, allowing you to compile it, customize it, and contribute to its development.
