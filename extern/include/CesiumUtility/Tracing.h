#pragma once

// If the build system doesn't enable the tracing support
// consider it disabled by default.
#ifndef CESIUM_TRACING_ENABLED
#define CESIUM_TRACING_ENABLED 0
#endif

#if !CESIUM_TRACING_ENABLED

#define CESIUM_TRACE_INIT(filename)
#define CESIUM_TRACE_SHUTDOWN()
#define CESIUM_TRACE(name)
#define CESIUM_TRACE_BEGIN(name)
#define CESIUM_TRACE_END(name)
#define CESIUM_TRACE_BEGIN_IN_TRACK(name)
#define CESIUM_TRACE_END_IN_TRACK(name)
#define CESIUM_TRACE_DECLARE_TRACK_SET(id, name)
#define CESIUM_TRACE_USE_TRACK_SET(id)
#define CESIUM_TRACE_LAMBDA_CAPTURE_TRACK() tracingTrack = false
#define CESIUM_TRACE_USE_CAPTURED_TRACK()

#else

// TODO: implement using Unreal tracing

#endif // CESIUM_TRACING_ENABLED
