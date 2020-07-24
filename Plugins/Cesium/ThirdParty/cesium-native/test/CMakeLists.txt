add_executable(cesium-native-tests "")

target_sources(
    cesium-native-tests
    PRIVATE
        test-main.cpp
)

target_include_directories(cesium-native-tests PUBLIC Cesium3DTiles CesiumGeometry CesiumUtility)
target_link_libraries(cesium-native-tests PUBLIC Cesium3DTiles CesiumGeometry CesiumUtility Catch2::Catch2)

set_target_properties(cesium-native-tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    # Use the multithreaded, release, DLL version of the runtime library no matter what.
    # This is appropriate for Unreal Engine, but needs to be configurable for other things.
    MSVC_RUNTIME_LIBRARY MultiThreadedDLL
)

add_subdirectory(Cesium3DTiles)

include(Catch)
catch_discover_tests(cesium-native-tests)
