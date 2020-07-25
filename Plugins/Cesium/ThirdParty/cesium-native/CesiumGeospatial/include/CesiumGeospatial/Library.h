#pragma once

#if defined(_WIN32) && defined(CESIUM_SHARED)
    #ifdef CESIUMGEOSPATIAL_BUILDING
        #define CESIUMGEOSPATIAL_API __declspec(dllexport)
    #else
        #define CESIUMGEOSPATIAL_API __declspec(dllimport)
    #endif
#else
    #define CESIUMGEOSPATIAL_API
#endif
