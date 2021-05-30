cmake_minimum_required(VERSION 3.15)

project(
    cesium-unreal-extern
    VERSION 0.1.0
    LANGUAGES CXX
)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CESIUM_ARCHITECTURE "64")
else()
    set(CESIUM_ARCHITECTURE "32")
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/../Source/ThirdParty)
set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_SYSTEM_NAME}-x${CESIUM_ARCHITECTURE})
add_subdirectory(cesium-native)

# Don't build the cesium-native tests by default
set_target_properties(cesium-native-tests PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
