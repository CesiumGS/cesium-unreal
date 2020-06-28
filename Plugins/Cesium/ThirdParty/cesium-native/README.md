# Cesium Native (for Unreal Engine)

## Prerequisites

* Visual Studio 2019
* CMake (add it to your path during install!)

## Getting Started

* Check out the repo with `git clone git@github.com:CesiumGS/cesium-native.git --recurse-submodules` so that you get the third party submodules.
* Build the draco library with CMake:
  * `pushd extern; mkdir build; cd build; mkdir draco; cd draco`
  * `cmake ../../draco/ -G "Visual Studio 16 2019"`
  * `cmake --build . --config Release`
  * `popd`
* Build the uriparser library with CMake:
  * `pushd extern; mkdir build; cd build; mkdir uriparser; cd uriparser`
  * `cmake ../../uriparser/ -G "Visual Studio 16 2019" -DCMAKE_BUILD_TYPE=Release -D URIPARSER_BUILD_TESTS:BOOL=OFF -D URIPARSER_BUILD_DOCS:BOOL=OFF -D BUILD_SHARED_LIBS:BOOL=OFF -D URIPARSER_ENABLE_INSTALL:BOOL=OFF -D URIPARSER_BUILD_TOOLS:BOOL=OFF`
  * `cmake --build . --config Release`
  * `popd`
* Open the cesium-native folder with Visual Studio Code with the `CMake Tools` extension installed. It should prompt you to generate project files from CMake. Choose `Visual Studio 2019 Release - amd64` as the kit to build. Then press Ctrl-Shift-P and execute the `CMake: Build` task or press F7. Alternatively, you can build from the command-line as follows:
  * `mkdir build`
  * `cmake -B build -S . -G "Visual Studio 16 2019"`
  * `cmake --build build --config Debug`
