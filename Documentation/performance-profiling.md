
Is your app running slowly? Here is a guide to help find performance problems in your code.

This guide profiles C++ code, running on Windows, using the Visual Studio CPU usage diagonostic tool ([link](https://learn.microsoft.com/en-us/visualstudio/profiling/beginners-guide-to-performance-profiling?view=vs-2022)).

## Set up a repeatable test

We need an area of code to execute repeatedly, with as many variables locked down as possible. 

In this example, we will use our Cesium performance tests.

#### Setup Unreal
1) Open Unreal Editor (UnrealEditor.exe)
2) Create a blank map (project doesn't matter. Choose an existing one or create a new one)
3) Go to Edit->Plugins
4) Search for "Functional Testing plugin". Check it to enable it
![smaller](https://github.com/CesiumGS/cesium-unreal/assets/130494071/5a3bc9de-cdaf-4d9d-842d-104719426663)
5) Save all
6) Set this map as the 'Editor Startup Map' so it loads when starting from Visual Studio
![smaller 2](https://github.com/CesiumGS/cesium-unreal/assets/130494071/8ba5c6c2-8c97-4048-afe2-db74770d85cc)


#### Setup your code

We need to make sure all our C++ code is building in release mode. Although you could profile a debug build, it is typically more useful to build in release, since this is how a game is usually packaged.

> This assumes that you have already built your code successfully and are familiar with the concepts from our [developer setup guide](https://github.com/CesiumGS/cesium-unreal/blob/ue5-main/Documentation/developer-setup-windows.md).

1) If building the cesium-native library, make sure you are using a Release build
2) Open your project's Visual Studio solution (.sln). In this example we will use the solution generated from https://github.com/CesiumGS/cesium-unreal-samples
3) Choose "Development Editor"

![smaller 3](https://github.com/CesiumGS/cesium-unreal/assets/130494071/0e70065f-c717-466b-a92b-cab1dcfdd29b)

4) From the menu, choose Build -> Build Solution


## Capture timing data

TODO

## Interpret the report

TODO
