cmake_minimum_required(VERSION 3.17)
project(
    cesium-native
    VERSION 0.1.0
    LANGUAGES CXX
)

# Use the multithreaded, release, DLL version of the runtime library no matter what.
# This is appropriate for Unreal Engine, but needs to be configurable for other things.
set(MSVC_RUNTIME_LIBRARY "MultiThreadedDLL" CACHE STRING "MSVC runtime library")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/extern/Catch2/contrib")

include(CTest)
enable_testing()

# set(CPACK_PROJECT_NAME ${PROJECT_NAME})
# set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
# include(CPack)

add_subdirectory(extern/Catch2)
add_subdirectory(extern/GSL)

# set(CMAKE_BUILD_TYPE ${ORIGINAL_CMAKE_BUILD_TYPE})
add_subdirectory(src)
add_subdirectory(test)
