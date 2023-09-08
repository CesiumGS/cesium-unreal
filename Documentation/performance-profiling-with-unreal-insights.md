This guide will help you find performance problems in your C++ code using [Unreal Insights](https://docs.unrealengine.com/5.0/en-US/unreal-insights-in-unreal-engine/), included with Unreal Engine.

Unreal Insights can display the scope of timing events as well as activity across threads. There is minimal impact to app execution, and you can set up your own custom events. It provides more functionality than an exclusive [CPU sampling-based profiler](https://learn.microsoft.com/en-us/visualstudio/profiling/understanding-performance-collection-methods-perf-profiler?view=vs-2022), although both tools can complement each other.

# Set up a repeatable test

We need an area of code to execute repeatedly, with as many variables locked down as possible. 

In this example, we will use our Cesium performance tests.

### Set up Unreal
1) Open Unreal Editor (UnrealEditor.exe)
2) Create a blank map (project doesn't matter. Choose an existing one or create a new one)
3) Go to Edit->Plugins
4) Search for "Functional Testing plugin". Check it to enable it
![smaller](https://github.com/CesiumGS/cesium-unreal/assets/130494071/5a3bc9de-cdaf-4d9d-842d-104719426663)
5) Save all
6) Set this map as the 'Editor Startup Map' so it loads when starting from Visual Studio
![smaller 2](https://github.com/CesiumGS/cesium-unreal/assets/130494071/8ba5c6c2-8c97-4048-afe2-db74770d85cc)


### Build Release Code

We need to make sure all our C++ code is building in release mode.

> This assumes that you have already built your code successfully and are familiar with the concepts from our [developer setup guide](https://github.com/CesiumGS/cesium-unreal/blob/ue5-main/Documentation/developer-setup-windows.md). Although you could profile a debug build, it is typically more useful to build in release, since this is how a game is usually packaged.

1) If building the cesium-native library, make sure you are using a Release configuration
2) Open your Unreal project's Visual Studio solution (.sln). This example uses the solution generated from [cesium-unreal-samples](https://github.com/CesiumGS/cesium-unreal-samples)
3) Choose "Development Editor"

![smaller 3](https://github.com/CesiumGS/cesium-unreal/assets/130494071/0e70065f-c717-466b-a92b-cab1dcfdd29b)

4) From the menu, choose Build -> Build Solution

# Prepare for capture

### Unreal Editor
1) In Visual Studio, click Debug -> Start Debugging (F5)
2) In Unreal, click Tools->Test Automation
3) Check the Cesium.Performance.LoadTestDenver row (don't start the test yet)
![Automation Window small](https://github.com/CesiumGS/cesium-unreal/assets/130494071/d27e7d67-3658-4cb2-ab10-777498cba0da)
4) Click Tools->Run Unreal Insights
5) In Unreal Insights, click on the "Connection" tab (don't connect yet)

> You can also find UnrealInsights.exe in UE_5.X\Engine\Binaries\Win64

![image](https://github.com/CesiumGS/cesium-unreal/assets/130494071/eadd4013-ca10-4b61-bb7d-0ab233440a39)

# Run the timing capture session
1) In Unreal Insights, click "Connect"
2) In Unreal Editor, click "Start Tests" (you should already have the Test Automation window open)
3) When the test ends, close Unreal Editor. We don't need it anymore.
4) In Unreal Insights, click the Trace Store tab, notice the trace that was just created
5) Click on it, then click on the 'Open Trace' button

> On the right side, there's a "Explore Trace Store Directory" button. You can click on this to delete or organize your traces

![image](https://github.com/CesiumGS/cesium-unreal/assets/130494071/f1e34fbc-35cd-4bc3-b935-5e322f5d9ba6)

# Interpret the report

![image](https://github.com/CesiumGS/cesium-unreal/assets/130494071/9cab7cf1-ab6d-4b58-a362-fc21ccff0334)

TODO

### Start at the timeline

TODO

### Examine low use areas

TODO

# Draw conclusions

TODO
