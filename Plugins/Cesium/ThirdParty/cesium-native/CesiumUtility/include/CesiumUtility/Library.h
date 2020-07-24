#pragma once

#if defined(_WIN32) && defined(CESIUM_SHARED)
    #ifdef CESIUMUTILITY_BUILDING
        #define CESIUMUTILITY_API __declspec(dllexport)
    #else
        #define CESIUMUTILITY_API __declspec(dllimport)
    #endif
#else
    #define CESIUMUTILITY_API
#endif
