# Using Pre-Release Packages

Cesium for Unreal has a continous integration (CI) system that automatically builds an installable version of the plugin for every commit in every branch. You can download and install these yourself to try out new features before they're released.

_Before you begin, **back up your project**. Cesium for Unreal can not guarantee that levels saved with a pre-release version will be loadable in other versions of the plugin._

## A pre-release version from a Pull Request

To download a pre-release version built from a pull request, click the "Checks" tab at the top of the pull request:

<img width="601" alt="image" src="https://github.com/CesiumGS/cesium-unreal/assets/924374/4e942591-b1b2-4d9d-9892-3ff05922ce48">

Then click the "Cesium for Unreal on: push" link:

<img width="320" alt="image" src="https://github.com/CesiumGS/cesium-unreal/assets/924374/f7e4d744-942c-436c-8cfa-402fb0e3071a">

And scroll down to the Artifacts section:

<img width="387" alt="image" src="https://github.com/CesiumGS/cesium-unreal/assets/924374/f2e57962-2bdf-4623-9856-a969378ceca2">

If there is no Artifacts section on that page, it's probably because the CI run is in progress. Artifacts don't appear until the build is complete.

Find the appropriate ZIP file for your version of Unreal Engine. For example, `CesiumForUnreal-52-*.zip` is for Unreal Engine 5.2. Note that artifacts that name a platform will _only_ work on that platform, while artifacts without any platform in the name will work on _all_ platforms that Cesium for Unreal supports. So in most cases you should download an artifact without any platform in its name.

Once you've downloaded the appropriate ZIP, move on to [installing a Cesium for Unreal ZIP](#installing-a-cesium-for-unreal-zip).

## Installing a Cesium for Unreal ZIP

Once you've download a pre-release ZIP file using one of the methods above, you can install it into your Unreal Engine as follows:

1. If you previously installed the Cesium for Unreal plugin via the Unreal Engine Marketplace, uninstall it first.
2. Find Unreal Engine's `Engine/Plugins/Marketplace` directory. For example, on Unreal Engine 5.2 on Windows, this is typically `C:\Program Files\Epic Games\UE_5.2\Engine\Plugins\Marketplace`. You may need to create the `Marketplace` directory yourself.
3. If the `CesiumForUnreal` subdirectory already exists in this `Marketplace` directory, delete it first to make sure you're getting a clean installation.
4. Extract the release ZIP into this `Marketplace` directory. f you've done this correctly, you'll find a `CesiumForUnreal` sub-directory inside the `Marketplace` directory.