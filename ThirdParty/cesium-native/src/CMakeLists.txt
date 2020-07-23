add_library(cesium-native "")

target_include_directories(
    cesium-native
    SYSTEM PUBLIC
        ${PROJECT_SOURCE_DIR}/extern/tinygltf
        ${PROJECT_SOURCE_DIR}/extern/glm
        ${PROJECT_SOURCE_DIR}/extern/draco/src
        ${PROJECT_SOURCE_DIR}/extern/uriparser/include
        ${PROJECT_SOURCE_DIR}/extern/build/draco
    PRIVATE
        private
)

target_link_libraries(cesium-native GSL)

if (MSVC)
    target_compile_options(cesium-native PRIVATE /W4 /WX)
else()
    target_compile_options(cesium-native PRIVATE -Werror -Wall -Wextra -Wconversion -Wpedantic)
endif()

set_target_properties(cesium-native PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    # Use the multithreaded, release, DLL version of the runtime library no matter what.
    # This is appropriate for Unreal Engine, but needs to be configurable for other things.
    MSVC_RUNTIME_LIBRARY MultiThreadedDLL
)

add_subdirectory(Cesium3DTiles)
