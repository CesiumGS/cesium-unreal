# 3D Tiles in Unreal Engine

The [Rendering 3D Tiles](\ref rendering-3d-tiles) page in the Cesium Native documentation explains in the abstract how Cesium Native can be used to integrate 3D Tiles rendering into an application. This page explains how Cesium for Unreal integrates 3D Tiles rendering into Unreal Engine specifically.

## ITaskProcessor

The implementation of [ITaskProcessor](\ref CesiumAsync::ITaskProcessor) for Unreal Engine is found in `UnrealTaskProcessor.h` and `.cpp` and is only a few lines of code. It uses Unreal's `AsyncTask` function to run the provided callback on the Unreal Engine task graph. The particular thread specifier used is `AnyBackgroundThreadNormalTask`. The meaning of this is not well documented, but we initially used `AnyThread` and found that our background work could sometimes block essential tasks related to rendering, causing frame-rate hiccups. Switching to `AnyBackgroundThreadNormalTask` slightly increased load times, but made rendering much smoother. See [CesiumGS/cesium-unreal#975](https://github.com/CesiumGS/cesium-unreal/pull/975) for further details.

## IAssetAccessor

`UnrealAssetAccessor` implements [IAssetAccessor](\ref CesiumAsync::IAssetAccessor) using Unreal's `HttpModule`. While this is largely straightforward and it has served us well overall, it has also been a source of quirks and performance problems.

##### File URLS

Unreal's HttpModule uses [libcurl](https://curl.se/libcurl/) under the hood, but the developers have chosen to disable libcurl's support for `file:///` URLs. Because our users frequently want to access 3D Tiles tileset from the local file system (in addition to the web), we have implemented custom support for file URLs in our asset accessor. It uses Unreal's `FFileHelper::LoadFileToArray` to read files, running in the `GIOThreadPool`.

##### Configuration Parameters

The Cesium for Unreal plugin includes `Config/Engine.ini` and `Config/Editor.ini` files to configure various aspects of Unreal's HTTP request system. See the comments in those files for an explanation of what we're changing and why. Without these tweaks, Unreal would spam the Output Log, complaining about Cesium making too many network requests. The time to download 3D Tiles files would also be much longer.

## IPrepareRendererResources

The implementation of the [IPrepareRendererResources](\ref Cesium3DTilesSelection::IPrepareRendererResources) interface in `UnrealPrepareRendererResources` is the heart of Cesium for Unreal. It is responsible for creating Unreal objects from 3D Tiles glTFs so that they can be rendered and interacted-with in Unreal Engine.

The major `UObject` classes involved in 3D Tiles rendering, and their inheritance relationships, are shown in the class diagram below. The types built into Unreal Engine are shown in a different color from the ones provided with Cesium for Unreal.

@mermaid{classes-for-3d-tiles}

* `ACesium3DTileset`: The Actor responsible for loading a 3D Tiles tileset. On each `Tick`, it calls Cesium Native's [updateView](\ref Cesium3DTilesSelection::Tileset::updateView).
* `UCesiumGltfComponent`: Represents a single 3D Tiles tile. A `ACesium3DTileset` will have many `UCesiumGltfComponent` instances attached to it, one for each tile that is currently loaded.
* `UCesiumGltfPrimitiveComponent`: Represents a single [MeshPrimitive](\ref CesiumGltf::MeshPrimitive) within a single 3D Tiles tile (glTF). A `UCesiumGltfComponent` will usually have one or more `UCesiumGltfPrimitiveComponent` instances attached to it.
* `UCesiumGltfPointsComponent`: A more specific type of `UCesiumGltfPrimitiveComponent` that is used when the `MeshPrimitive` uses the [POINTS](\ref CesiumGltf::MeshPrimitive::Mode::POINTS) mode. That is, when it is a point cloud.
* `UCesiumGltfInstancedComponent`: An alternate representation of a glTF `MeshPrimitive` that is used when multiple copies of the mesh are rendered under the direction of the [EXT_gpu_insta`ncing](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Vendor/EXT_mesh_gpu_instancing) extension.

Instances of `UCesiumGltfComponent`, `UCesiumGltfInstancedComponent`, and `UCesiumGltfPointsComponent`, along with accompanying `UStaticMesh`, `UMaterialInstanceDynamic`, and `UTexture2D` instances, are created by `UnrealPrepareRendererResources` as 3D Tiles are loaded. These `UObject`-derived classes are created on the game thread. Game thread time is a limited resource in most Unreal Engine applications, though, so we strive to do as much of the loading work as possible in background threads.

For that reason, Cesium for Unreal takes advantage of the separate [prepareInLoadThread](\ref Cesium3DTilesSelection::IPrepareRendererResources::prepareInLoadThread) and [prepareInMainThread](\ref Cesium3DTilesSelection::IPrepareRendererResources::prepareInMainThread) methods on `IPrepareRendererResources`.

> [!note]
> Recent versions of Unreal Engine reportedly do allow creating UObjects in background threads, probably with some significant caveats. In the future, we should consider whether the relaxation of this limitation allows us to improve our design or performance.

In `prepareInLoadThread`, Cesium for Unreal receives a [Model](\ref CesiumGltf::Model) from Cesium Native and creates from it a `LoadedModelResult`. This struct holds a representation of the model that is as close to "fully renderable in Unreal Engine" as we can manage without creating any `UObjects`:

* Normals and tangents are generated, if required.
* Texture MipMaps are generated, if required.
* Physics meshes are generated, if required.
* Mesh vertex and index data for each `MeshPrimitive` are copied into an instance of Unreal's `FStaticMeshRenderData`.
* An `FCesiumTextureResource` is created for each texture. This class is derived from Unreal's `FTextureResource` and is Unreal's low-level, render-thread representation of a texture.
* Feature IDs and metadata that are made available to a material via the [UCesiumEncodedMetadataComponent](\ref UCesiumEncodedMetadataComponent) are turned into additional textures.

Then, in `prepareInMainThread`, we receive the `LoadedModelResult` produced above, and create all of the `UObject` instances from it, avoiding as much as possible copying or transforming any data.

### Multithreaded Texture Creation

Textures are often the largest part of a 3D Tiles tileset, especially for real-world, photogrammetry-derived models. So, Cesium for Unreal takes great pains to:

1. Avoid keeping multiple copies of a texture in CPU or GPU memory.
2. Avoid copying texture data unnecessarily, even if it's only held temporarily.
3. Create renderable textures on the GPU without using any more game thread or render thread time than is absolutely necessary.

To that end, Cesium for Unreal's texture creation system uses some low-level, largely undocumented parts of the Unreal Engine API. This is made trickier by the fact that a single image may have multiple purposes within a glTF, or it may be shared across multiple glTF tiles in a 3D Tiles tileset.

Textures are created near the start of `prepareInLoadThread` with calls to `ExtensionImageAssetUnreal::getOrCreate`. This method expects that multiple threads may call it simultaneously on a single [ImageAsset](\ref CesiumGltf::ImageAsset). This happens when the two threads are loading two different tiles that happen to share a single image. It uses a mutex to ensure that only the first thread adds an `ExtensionImageAssetUnreal` and then proceeds to load the image. Any other threads will instead get the existing `ExtensionImageAssetUnreal`, which includes a [SharedFuture](\ref CesiumAsync::SharedFuture) that will resolve when the first thread has finished loading the image.

This system ensures that a) only one thread loads the image, and b) other threads can asynchronously wait for the image to be loaded, without blocking any threads while they're waiting.

The thread that is doing the actual loading will create a new instance of a class derived from `FCesiumTextureResource` for the `ImageAsset`.

@mermaid{texture-resource-classes}

For Unreal Render Hardware Interfaces (RHI) that support asynchronous texture upload (`GRHISupportsAsyncTextureCreation` is set), which is currently only Direct3D 11 and 12, the instance will be of type `FCesiumPreCreatedRHITextureResource`. The GPU upload will happen immediately under the control of the worker thread with a call to `RHIAsyncCreateTexture2D`.

For RHIs that don't support async texture upload, the new instance will be of type `FCesiumCreateNewTextureResource` and a command will be queued to the render thread to do the texture upload with a call to `RHICreateTexture`. In either case, the CPU-side pixel data, which is now owned by the `FCesiumTextureResource`, is freed once the GPU upload is complete.

In Unreal Engine, an `FTextureResource` encapsulates both the GPU texture resource (represented as `FRHITexture`), but also details such as the sampling mode (nearest, linear, mipmaps), as well as whether or not to treat it as sRGB. This is unfortunate because we would like to have just one copy of each set of pixel data, even if that pixel data happens to be sampled differently when it's used in different contexts. Fortunately, we can create multiple `FTextureResource` instances that reference a single `FRHITexture` via reference counting.

For simplicity, we always do this, even when there is no sharing. The initial `FCesiumTextureResource` that is created can be viewed as a representation of the glTF [Image](\ref CesiumGltf::Image). Then, we create an instance of `FCesiumUseExistingTextureResource` for each glTF [Texture](\ref CesiumGltf::Texture). The `FCesiumUseExistingTextureResource` wraps the `FCesiumCreateNewTextureResource` or `FCesiumPreCreatedRHITextureResource` instance created previously and brings the sampler settings that are appropriate for the context.
