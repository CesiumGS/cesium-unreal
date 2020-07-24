cmake_minimum_required(VERSION 3.15)
project(
    cesium-native
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/extern/Catch2/contrib")

include(CTest)
enable_testing()

add_subdirectory(extern/Catch2)
add_subdirectory(extern/GSL)

function(configure_cesium_library targetName)
    if (MSVC)
        target_compile_options(${targetName} PRIVATE /W4 /WX)
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

    target_compile_definitions(
        ${targetName}
        PUBLIC
            CESIUM3DTILES_API=
    )   
endfunction()

add_subdirectory(src)
add_subdirectory(test)
