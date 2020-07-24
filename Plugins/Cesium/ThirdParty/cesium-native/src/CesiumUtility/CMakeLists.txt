add_library(CesiumUtility "")

file(GLOB SOURCES *.cpp *.h)
file(GLOB PUBLIC_INCLUDES ${PROJECT_SOURCE_DIR}/include/CesiumUtility/*.h)

target_sources(
    CesiumUtility
    PRIVATE
        ${SOURCES}
    PUBLIC
        ${PUBLIC_INCLUDES}
)

target_include_directories(
    CesiumUtility
    SYSTEM PUBLIC
        ${PROJECT_SOURCE_DIR}/extern/glm
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(
    CesiumUtility
    PUBLIC
        CESIUM3DTILES_API=
)

if (MSVC)
    target_compile_options(CesiumUtility PRIVATE /W4 /WX)
else()
    target_compile_options(CesiumUtility PRIVATE -Werror -Wall -Wextra -Wconversion -Wpedantic -Wshadow)
endif()

set_target_properties(CesiumUtility PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    # Use the multithreaded, release, DLL version of the runtime library no matter what.
    # This is appropriate for Unreal Engine, but needs to be configurable for other things.
    MSVC_RUNTIME_LIBRARY MultiThreadedDLL
)
