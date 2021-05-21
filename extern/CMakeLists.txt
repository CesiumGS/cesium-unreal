cmake_minimum_required(VERSION 3.15)

project(
    cesium-unreal-extern
    VERSION 0.1.0
    LANGUAGES CXX C
)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    set(CESIUM_ARCHITECTURE "aarch64")
    set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF)
else()
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CESIUM_ARCHITECTURE "64")
    else()
        set(CESIUM_ARCHITECTURE "32")
    endif()
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/../Source/ThirdParty)
set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_SYSTEM_NAME}-x${CESIUM_ARCHITECTURE})
add_subdirectory(cesium-native)

# cesium-native doesn't require this header to be public, but Cesium for Unreal wants to use it.
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cesium-native/extern/stb/stb_image_resize.h TYPE INCLUDE)

# Build and install MikkTSpace because we need it on Android.
set(CESIUM_DEBUG_POSTFIX "d")
set(CESIUM_RELEASE_POSTFIX "")

set(CMAKE_DEBUG_POSTFIX ${CESIUM_DEBUG_POSTFIX})
set(CMAKE_RELEASE_POSTFIX ${CESIUM_RELEASE_POSTFIX})
set(CMAKE_MINSIZEREL_POSTFIX ${CESIUM_RELEASE_POSTFIX})
set(CMAKE_RELWITHDEBINFO_POSTFIX ${CESIUM_RELEASE_POSTFIX})

add_library(MikkTSpace MikkTSpace/mikktspace.c)
set_target_properties(MikkTSpace PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/MikkTSpace/mikktspace.h")
target_include_directories(MikkTSpace PUBLIC MikkTSpace)
install(TARGETS MikkTSpace
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Don't build the cesium-native tests by default
set_target_properties(cesium-native-tests PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
