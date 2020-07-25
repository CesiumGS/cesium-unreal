cmake_minimum_required(VERSION 3.15)
project(
    cesium-native
    VERSION 0.1.0
    LANGUAGES CXX
)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/extern/Catch2/contrib")

include(CTest)
enable_testing()

add_subdirectory(extern/Catch2)
add_subdirectory(extern/GSL)

function(configure_cesium_library targetName)
    if (MSVC)
        target_compile_options(${targetName} PRIVATE /W4 /WX /wd4201)
    else()
        target_compile_options(${targetName} PRIVATE -Werror -Wall -Wextra -Wconversion -Wpedantic -Wshadow)
    endif()

    set_target_properties(${targetName} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        # Use the multithreaded, release, DLL version of the runtime library no matter what.
        # This is appropriate for Unreal Engine, but needs to be configurable for other things.
        MSVC_RUNTIME_LIBRARY MultiThreadedDLL
    )

    string(TOUPPER ${targetName} capitalizedTargetName)

    target_compile_definitions(
        ${targetName}
        PRIVATE
            ${capitalizedTargetName}_BUILDING
    )

    if (BUILD_SHARED_LIBS)
        target_compile_definitions(
            ${targetName}
            PUBLIC
                CESIUM_SHARED=${BUILD_SHARED_LIBS}
        )
    endif()

    if ((TARGET cesium-native-tests) AND (NOT ${targetName} MATCHES "cesium-native-tests"))
        target_link_libraries(cesium-native-tests PUBLIC ${targetName})

        file(GLOB TEST_SOURCES test/*.cpp test/*.h)
        
        target_sources(
            cesium-native-tests
            PRIVATE
                ${TEST_SOURCES}
        )
    endif()   
endfunction()

add_executable(cesium-native-tests "")
configure_cesium_library(cesium-native-tests)

target_sources(
    cesium-native-tests
    PRIVATE
        test-main.cpp
)

target_link_libraries(cesium-native-tests PUBLIC Catch2::Catch2)

add_subdirectory(CesiumUtility)
add_subdirectory(CesiumGeometry)
add_subdirectory(CesiumGeospatial)
add_subdirectory(Cesium3DTiles)

include(Catch)
catch_discover_tests(cesium-native-tests)
