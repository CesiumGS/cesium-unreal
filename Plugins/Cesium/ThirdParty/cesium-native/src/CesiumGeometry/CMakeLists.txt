add_library(CesiumGeometry "")

if (MSVC)
    target_compile_options(CesiumGeometry PRIVATE /W4 /WX)
else()
    target_compile_options(CesiumGeometry PRIVATE -Werror -Wall -Wextra -Wconversion -Wpedantic -Wshadow)
endif()

set_target_properties(CesiumGeometry PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    # Use the multithreaded, release, DLL version of the runtime library no matter what.
    # This is appropriate for Unreal Engine, but needs to be configurable for other things.
    MSVC_RUNTIME_LIBRARY MultiThreadedDLL
)

file(GLOB SOURCES *.cpp *.h)
file(GLOB PUBLIC_INCLUDES ${PROJECT_SOURCE_DIR}/include/CesiumGeometry/*.h)

target_sources(
    CesiumGeometry
    PRIVATE
        ${SOURCES}
    PUBLIC
        ${PUBLIC_INCLUDES}
)

target_include_directories(
    CesiumGeometry
    SYSTEM PUBLIC
        ${PROJECT_SOURCE_DIR}/extern/glm
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        private
)

target_link_libraries(CesiumGeometry CesiumUtility GSL)

target_compile_definitions(
    CesiumGeometry
    PUBLIC
        CESIUM3DTILES_API=
)
