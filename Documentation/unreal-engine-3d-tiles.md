# 3D Tiles in Unreal Engine

The [Rendering 3D Tiles](\ref rendering-3d-tiles) page in the Cesium Native documentation explains in the abstract how Cesium Native can be used to integrate 3D Tiles rendering into an application. This page explains how Cesium for Unreal integrates 3D Tiles rendering into Unreal Engine specifically.

## ITaskProcessor

The implementation of [ITaskProcessor](\ref CesiumAsync::ITaskProcessor) for Unreal Engine is found in `UnrealTaskProcessor.h` and `.cpp` and is only a few lines of code. It uses Unreal's `AsyncTask` function to run the provided callback on the Unreal Engine task graph. The particular thread specifier used is `AnyBackgroundThreadNormalTask`. The meaning of this is not well documented, but we initially used `AnyThread` and found that our background work could sometimes block essential tasks related to rendering, causing frame-rate hiccups. Switching to `AnyBackgroundThreadNormalTask` slightly increased load times, but made rendering much smoother. See [CesiumGS/cesium-unreal#975](https://github.com/CesiumGS/cesium-unreal/pull/975) for further details.

## IAssetAccessor

`UnrealAssetAccessor` implements [IAssetAccessor](\ref CesiumAsync::IAssetAccessor) using Unreal's `HttpModule`. While this is largely straightforward and it has served us well overall, it has also been a source of numerous quirks and performance problem.

#### File URLS

While Unreal's HttpModule uses [libcurl](https://curl.se/libcurl/) under the hood, the developers have chosen to disable libcurl's support for `file:///` URLs. Because our users frequently want to access 3D Tiles tileset from the local file system, we have implemented custom support for file URLs in our asset accessor. It uses Unreal's `FFileHelper::LoadFileToArray` to read files, running in the `GIOThreadPool`.

#### Configuration Parameters

The Cesium for Unreal plugin includes `Config/Engine.ini` and `Config/Editor.ini` files to configure various aspects of Unreal's HTTP request system. See the comments in those files for an explanation of what we're changing and why. Without these tweaks, Unreal would spam the Output Log, complaining about Cesium making too many network requests, and the time to download 3D Tiles files would be much longer.

## IPrepareRendererResources

The implementation of the [IPrepareRendererResources](\ref Cesium3DTilesSelection::IPrepareRendererResources) interface in `UnrealPrepareRendererResources` is the heart of Cesium for Unreal. It is responsible for creating Unreal objects from 3D Tiles glTFs so that they can be rendered and interacted-with in Unreal Engine.

The major `UObject` classes involved in 3D Tiles rendering, and their inheritance relationships, are shown in the class diagram below. The types built into Unreal Engine are shown in a different color.

@mermaid{classes-for-3d-tiles}

These, along with `UStaticMesh`, `UMaterialInstanceDynamic` and `UTexture2D` instances, are created in the course of loading 3D Tiles. Unreal Engine usually requires that these `UObject`-derived classes be created in the game thread.

> [!note]
> Recent versions of Unreal Engine reportedly do allow creating UObjects in background threads, probably with some significant caveats. In the future, we should consider whether the relaxation of this limitation allows us to improve our design or performance.

