
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


#### Build Release Code

We need to make sure all our C++ code is building in release mode.

> This assumes that you have already built your code successfully and are familiar with the concepts from our [developer setup guide](https://github.com/CesiumGS/cesium-unreal/blob/ue5-main/Documentation/developer-setup-windows.md). Although you could profile a debug build, it is typically more useful to build in release, since this is how a game is usually packaged.

1) If building the cesium-native library, make sure you are using a Release configuration
2) Open your Unreal project's Visual Studio solution (.sln). This example uses the solution generated from https://github.com/CesiumGS/cesium-unreal-samples
3) Choose "Development Editor"

![smaller 3](https://github.com/CesiumGS/cesium-unreal/assets/130494071/0e70065f-c717-466b-a92b-cab1dcfdd29b)

4) From the menu, choose Build -> Build Solution


## Setup Visual Studio for capture

1) Open your project's Visual Studio solution (.sln). This example uses the solution generated from https://github.com/CesiumGS/cesium-unreal-samples
2) From the menu, choose Debug->Windows->Show Diagnostic Tools
3) Configure it. Uncheck 'Memory Usage'. Under Settings, Uncheck "Enable CPU Profiling", we'll turn this back on later.

<img width="484" alt="DiagSetup" src="https://github.com/CesiumGS/cesium-unreal/assets/130494071/798d794c-19f0-4f15-93e1-1815e3f1e75b">

4) Optionally, find two places in your code to set breakpoints. In our example, performance tests already log at their timing marks. Let's set a breakpoint at the start and end.

![Breakpoint Set small](https://github.com/CesiumGS/cesium-unreal/assets/130494071/5a793b9c-fd68-42ed-96ae-6ec884c38951)

> We don't have to do this. We could profile the entire debugging session if we needed to. But it's generally good practice to reduce your timing capture as much as possible. This can improve responsiveness when using profiling tools, especially when dealing with intensive operations like memory heap tracking. 

## Run the timing capture session

1) Start your debugging session (Debug->Start Debugging, F5)
2) Find the performance tests in Unreal. Tools->Test Automation
![Automation Window small](https://github.com/CesiumGS/cesium-unreal/assets/130494071/d27e7d67-3658-4cb2-ab10-777498cba0da)

3) Check "LoadTestDenver"
4) Click "Start Tests"
5) Your first break point should hit in Visual Studio
6) Go back to the Diagnostic Tools window, click on "Record CPU Profile". It should turn red
![image](https://github.com/CesiumGS/cesium-unreal/assets/130494071/ce0c7e86-c1ef-4a01-97fd-c97275b6f62b)

7) Continue the debugging session (Debug->Continue, F5)
8) Your second break point should hit
9) Go back to the Diagnostic Tools window, you should now see a report

## Interpret the report

![image](https://github.com/CesiumGS/cesium-unreal/assets/130494071/b83c63e0-06c4-47ff-afab-17a9923fa646)

TODO
