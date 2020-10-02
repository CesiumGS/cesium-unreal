cmake_minimum_required(VERSION 3.15)

project(
    cesium-unreal-extern
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
add_subdirectory(cesium-native)
