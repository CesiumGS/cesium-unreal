add_library(Cesium3DTiles "")

if (MSVC)
    target_compile_options(Cesium3DTiles PRIVATE /W4 /WX)
else()
    target_compile_options(Cesium3DTiles PRIVATE -Werror -Wall -Wextra -Wconversion -Wpedantic -Wshadow)
endif()

set_target_properties(Cesium3DTiles PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    # Use the multithreaded, release, DLL version of the runtime library no matter what.
    # This is appropriate for Unreal Engine, but needs to be configurable for other things.
    MSVC_RUNTIME_LIBRARY MultiThreadedDLL
)

file(GLOB SOURCES *.cpp *.h)
file(GLOB PUBLIC_INCLUDES ${PROJECT_SOURCE_DIR}/include/Cesium3DTiles/*.h)

target_sources(
    Cesium3DTiles
    PRIVATE
        ${SOURCES}
    PUBLIC
        ${PUBLIC_INCLUDES}
)

target_include_directories(
    Cesium3DTiles
    SYSTEM PUBLIC
        ${PROJECT_SOURCE_DIR}/extern/tinygltf
        ${PROJECT_SOURCE_DIR}/extern/glm
        ${PROJECT_SOURCE_DIR}/extern/draco/src
        ${PROJECT_SOURCE_DIR}/extern/uriparser/include
        ${PROJECT_SOURCE_DIR}/extern/build/draco
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        private
)

target_link_libraries(Cesium3DTiles GSL)

target_compile_definitions(
    Cesium3DTiles
    PUBLIC
        CESIUM3DTILES_API=
)
