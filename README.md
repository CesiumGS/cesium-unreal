# Cesium for Unreal Engine

## Prerequisites

* Visual Studio 2017
* CMake (add it to your path during install!)

## Getting Started

* Check out the repo with `git clone git@github.com:CesiumGS/cesium-unreal.git --recurse-submodules` so that you get the third party submodules.
* Build the draco library with CMake:
  * `cd Plugins/Cesium/ThirdParty; mkdir build; cd build; mkdir draco; cd draco`
  * `cmake ../../draco/ -G "Visual Studio 15 2017 Win64"`
  * `cmake --build . --config Release`
* Build the uriparser library with CMake:
  * `cd Plugins/Cesium/ThirdParty; mkdir build; cd build; mkdir uriparser; cd uriparser`
  * `cmake ../../uriparser/ -G "Visual Studio 15 2017 Win64" -DCMAKE_BUILD_TYPE=Release -D URIPARSER_BUILD_TESTS:BOOL=OFF -D URIPARSER_BUILD_DOCS:BOOL=OFF -D BUILD_SHARED_LIBS:BOOL=OFF -D URIPARSER_ENABLE_INSTALL:BOOL=OFF -D URIPARSER_BUILD_TOOLS:BOOL=OFF`
  * `cmake --build . --config Release`
* Open cesiumunreal.uproject in the Unreal Editor.
* Say "yes" when prompted to rebuild `cesiumunreal` and `Cesium`.
* Generate Visual Studio project files manually by choosing `File -> [Refresh/Generate] Visual Studio Project` in the editor.
* Open the project in Visual Studio by going to `File -> Open Visual Studio`

## Tips

### Build errors on open

After loading the project in Unreal Editor and click "yes" to rebuild, something could go wrong preventing the project from building. And when that happens, it's hard to see what happened because the Editor closes before you can see the log. To work around this, right-click on the `cesiumunreal.uproject` file and click "Generate Visual Studio Project files". Then open the generated cesiumunreal.sln in Visual Studio and compile it there.

### Hot Reloading

Changes made to the game project (cesiumunreal), when compiled, will be hot-reloaded by the Unreal Editor. But this doesn't work for changes in the plugin (Cesium). To build and reload plugin changes without restarting the editor, go to Window -> Developer Tools -> Modules, search for Cesium, and click the "Recompile" button next to it.
