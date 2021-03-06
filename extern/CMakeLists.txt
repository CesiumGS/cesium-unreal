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

set(CESIUM_DEBUG_POSTFIX "-${CMAKE_SYSTEM_NAME}-${CESIUM_ARCHITECTURE}-debug")
set(CESIUM_RELEASE_POSTFIX "-${CMAKE_SYSTEM_NAME}-${CESIUM_ARCHITECTURE}-release")

set(CMAKE_DEBUG_POSTFIX ${CESIUM_DEBUG_POSTFIX})
set(CMAKE_RELEASE_POSTFIX ${CESIUM_RELEASE_POSTFIX})
set(CMAKE_MINSIZEREL_POSTFIX ${CESIUM_RELEASE_POSTFIX})
set(CMAKE_RELWITHDEBINFO_POSTFIX ${CESIUM_RELEASE_POSTFIX})

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/../Source/ThirdParty)
add_subdirectory(cesium-native)

# These libraries override the debug postfix, so re-override it.
set_target_properties(spdlog PROPERTIES DEBUG_POSTFIX ${CESIUM_DEBUG_POSTFIX})
set_target_properties(spdlog PROPERTIES RELEASE_POSTFIX ${CESIUM_RELEASE_POSTFIX})
set_target_properties(spdlog PROPERTIES MINSIZEREL_POSTFIX ${CESIUM_RELEASE_POSTFIX})
set_target_properties(spdlog PROPERTIES RELWITHDEBINFO_POSTFIX ${CESIUM_RELEASE_POSTFIX})
set_target_properties(tinyxml2 PROPERTIES DEBUG_POSTFIX ${CESIUM_DEBUG_POSTFIX})
set_target_properties(tinyxml2 PROPERTIES RELEASE_POSTFIX ${CESIUM_RELEASE_POSTFIX})
set_target_properties(tinyxml2 PROPERTIES MINSIZEREL_POSTFIX ${CESIUM_RELEASE_POSTFIX})
set_target_properties(tinyxml2 PROPERTIES RELWITHDEBINFO_POSTFIX ${CESIUM_RELEASE_POSTFIX})

# cesium-native doesn't require this header to be public, but Cesium for Unreal wants to use it.
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cesium-native/extern/stb/stb_image_resize.h TYPE INCLUDE)
