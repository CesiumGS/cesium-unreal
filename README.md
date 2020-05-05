# Cesium for Unreal Engine

## Getting Started

* Check out the repo with `git clone git@github.com:CesiumGS/cesium-unreal.git --recurse-submodules` so that you get the tinygltf submodule.
* Open cesiumunreal.uproject in the Unreal Editor.
* Generate Visual Studio project files by choosing `File -> Refresh Visual Studio Project`.
* Open the project in Visual Studio by going to `File -> Open Visual Studio`

## Tips

### Hot Reloading

Changes made to the game project (cesiumunreal), when compiled, will be hot-reloaded by the Unreal Editor. But this doesn't work for changes in the plugin (Cesium). To build and reload plugin changes without restarting the editor, go to Window -> Developer Tools -> Modules, search for Cesium, and click the "Recompile" button next to it.
