
This guide will help you find performance problems in your C++ code using the [CPU Usage tool](https://learn.microsoft.com/en-us/visualstudio/profiling/beginners-guide-to-performance-profiling?view=vs-2022) included in Visual Studio's Diagnostic tools window.

The CPU Usage tool is easy to set up with minimal impact on how your app is built or how it runs. If you use Visual Studio often, you may have this running already. This is a sampling-based profiler, with pros and cons detailed [here](https://learn.microsoft.com/en-us/visualstudio/profiling/understanding-performance-collection-methods-perf-profiler?view=vs-2022).

# Set up a repeatable test

In this example, we will use our Cesium performance tests. Follow the steps outlined [here](https://github.com/CesiumGS/cesium-unreal/blob/profiling-documentation/Documentation/performance-profiling-setup-test.md).

# Prepare for capture

### Visual Studio

1) Open your project's Visual Studio solution (.sln). This example uses the solution generated from [cesium-unreal-samples](https://github.com/CesiumGS/cesium-unreal-samples)
2) From the menu, choose Debug->Windows->Show Diagnostic Tools
3) Configure it. Uncheck 'Memory Usage'. Under Settings, Uncheck "Enable CPU Profiling", we'll turn this back on later.

<img width="484" alt="DiagSetup" src="https://github.com/CesiumGS/cesium-unreal/assets/130494071/798d794c-19f0-4f15-93e1-1815e3f1e75b">

4) Optionally, find two places in your code to set breakpoints. In our example, performance test start / end marks are perfect. 

![Breakpoint Set small](https://github.com/CesiumGS/cesium-unreal/assets/130494071/5a793b9c-fd68-42ed-96ae-6ec884c38951)

>We could profile the entire debugging session if we needed to. But it's generally good practice to reduce your timing capture as much as possible. This can improve responsiveness when using resource intensive profiling tools, like memory tracking. 

# Run the timing capture session

1) From Visual Studio, start your debugging session (Debug->Start Debugging, F5)
2) Find the performance tests in Unreal. Tools->Test Automation
![Automation Window small](https://github.com/CesiumGS/cesium-unreal/assets/130494071/d27e7d67-3658-4cb2-ab10-777498cba0da)

3) Check "LoadTestDenver"
4) Click "Start Tests"
5) Your first break point should hit in Visual Studio
6) Go back to the Diagnostic Tools window, click on "Record CPU Profile". It should turn red.

![image](https://github.com/CesiumGS/cesium-unreal/assets/130494071/ce0c7e86-c1ef-4a01-97fd-c97275b6f62b)

7) Continue the debugging session (Debug->Continue, F5)
8) Your second break point should hit
9) Go back to the Diagnostic Tools window, you should now see a report

# Interpret the report

![image](https://github.com/CesiumGS/cesium-unreal/assets/130494071/a9fb3e0b-86f5-4239-b4ab-c7f9b1dba4a5)

This can be a bit daunting at first, but most profiling tools have a similar workflow.

### Start at the timeline

![image](https://github.com/CesiumGS/cesium-unreal/assets/130494071/da733adc-6cae-4c89-8a6c-01a367667a0d)

Note the highlighted area to the right where the CPU usage spikes. This corresponds to the breakpoints that we set. 

All data from the report will reflect this selection only.

### Trace calls with high usage

From the main window, click on "Open Details"
![image](https://github.com/CesiumGS/cesium-unreal/assets/130494071/f34b5ee3-15b7-485a-a90a-8f71310b1b44)

The CPU Usage window will appear. Set "Current View" to "Functions", then find the 'Self CPU' column and sort descending (down arrow).

![image](https://github.com/CesiumGS/cesium-unreal/assets/130494071/20836178-8337-4d53-be58-f388db905f9f)

This window now shows the functions that have the highest time spent within themselves only. Useful for finding individual functions that are called often or need to be optimized. 

In this example, `stbir_resample_horizontal_downsample` is of particular interest because it's in the code base we built. Entries with `[External]` or originate from an unfamiliar module are generally ignored, although it is useful to know we are calling into them. 

Right click on the  `stbir_resample_horizontal_downsample` row, select "View in Call Tree".

![image](https://github.com/CesiumGS/cesium-unreal/assets/130494071/e5a88c06-5b76-4a07-83be-db5147a961b2)

# Draw conclusions

The window above is starting to show some actionable information:
- 20% of the sampled CPU time was spent in `CesiumTextureUtility::loadTextureAnyThreadPart`. Basically, we're loading textures
- There are a variety of `stbir_XXX functions` that are taking the bulk of the time, and might be candidates for optimization
- The highest cost single function is `stbir_resample_horizontal_downsample`, but keep in mind the limits of a sampling profiler. We don't know how many times it was called, just that it was being executed ~6% of the time.

Are these functions worth investigating and potentially optimizing? Maybe. Again, know this is a sampling profiler. Even if you optimize the highest cost function to 0.001%, you are only improving CPU efficiency. 

If your goal is to reach absolute numbers, like specific loading times, or frames per second, you may need another type of profiling tool.
