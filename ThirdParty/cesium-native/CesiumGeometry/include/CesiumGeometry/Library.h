#pragma once

#if defined(_WIN32) && defined(CESIUM_SHARED)
    #ifdef CESIUMGEOMETRY_BUILDING
        #define CESIUMGEOMETRY_API __declspec(dllexport)
    #else
        #define CESIUMGEOMETRY_API __declspec(dllimport)
    #endif
#else
    #define CESIUMGEOMETRY_API
#endif
