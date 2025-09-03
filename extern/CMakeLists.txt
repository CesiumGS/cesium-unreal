cmake_minimum_required(VERSION 3.15)

project(
    cesium-unreal-extern
    VERSION 0.1.0
    LANGUAGES CXX C
)

if (DEFINED ENV{UNREAL_ENGINE_ROOT} AND NOT UNREAL_ENGINE_ROOT)
  set(UNREAL_ENGINE_ROOT "$ENV{UNREAL_ENGINE_ROOT}")
endif()

if (NOT UNREAL_ENGINE_ROOT)
  # On Windows and Mac, try to use the default UE 5.4 location
  if (WIN32)
     set(DEFAULT_UNREAL_INSTALLATION "C:/Program Files/Epic Games/UE_5.4")
  elseif(APPLE)
    set(DEFAULT_UNREAL_INSTALLATION "/Users/Shared/Epic Games/UE_5.4")
  endif()

  if (NOT EXISTS "${DEFAULT_UNREAL_INSTALLATION}")
     message(FATAL_ERROR "Please specify the root of your Unreal Engine installation, either by setting the UNREAL_ENGINE_ROOT environment variable or with -DUNREAL_ENGINE_ROOT=path on the cmake command-line.")
  else()
    set(UNREAL_ENGINE_ROOT "${DEFAULT_UNREAL_INSTALLATION}")
  endif()
endif()

message(STATUS "Using Unreal Engine installation at: ${UNREAL_ENGINE_ROOT}")

# Set the UE environment variable so it's inherited by vcpkg builds.
set(ENV{UNREAL_ENGINE_ROOT} "${UNREAL_ENGINE_ROOT}")

if (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    set(CESIUM_ARCHITECTURE "aarch64")
    set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF)
    set(ANDROID_ABI ${CMAKE_ANDROID_ARCH_ABI})
    set(ANDROID_NDK ${CMAKE_ANDROID_NDK})
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
    set(CESIUM_ARCHITECTURE "arm64")
else()
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CESIUM_ARCHITECTURE "64")
    else()
        set(CESIUM_ARCHITECTURE "32")
    endif()
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
    # Fix for PThread library not in path
    set(CMAKE_THREAD_LIBS_INIT "-lpthread")
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
    set(CMAKE_USE_PTHREADS_INIT 1)

    find_package(Threads REQUIRED)
endif()

# Rename sqlite3* symbols to cesium_sqlite3* so they don't conflict with UE's sqlite3,
# which has a bunch of limitations and is not considered public.
set(PRIVATE_CESIUM_SQLITE ON)

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/../Source/ThirdParty)
set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}-$<IF:$<CONFIG:Debug>,Debug,Release>)

# Always define NDEBUG, since Unreal does in debug anyway.
# We don't want any mismatches, especially with class member ordering.
# But still let debug builds force assertions.
add_compile_definitions(NDEBUG $<$<CONFIG:Debug>:CESIUM_FORCE_ASSERTIONS>)

# On Mac and Linux, Unreal uses -fvisibility-ms-compat.
# On Android, it uses -fvisibility=hidden
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-ms-compat -fvisibility-inlines-hidden -fno-rtti")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden -fno-rtti")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fno-rtti")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  # Unreal Engine adds /Zp8 in 64-bit Windows builds to align structs to 8 bytes instead of the
  # default of 16 bytes. There's this nice note in the documentation for that option:
  #   Don't change the setting from the default when you include the Windows SDK headers, either
  #   by using /Zp on the command line or by using #pragma pack. Otherwise, your application may
  #   cause memory corruption at runtime.
  # (https://docs.microsoft.com/en-us/cpp/build/reference/zp-struct-member-alignment?view=msvc-160)
  # Yeah that's not just the Windows SDK, but anything that passes structs across the boundary
  # between compilation units using different versions of that flag. We compile cesium-native
  # with this same option to avoid super-dodgy and hard to debug issues.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zp8")
endif()

option(CESIUM_USE_UNREAL_TRACING "Whether to trace cesium-native events using Unreal's tracing framework.")
if (CESIUM_USE_UNREAL_TRACING)
  # Trace Cesium Native with Unreal Insights. Need to include Unreal Engine headers when building
  # Cesium Native.

  # Change this line to change engine version. Note you will have to set an environment variable to
  # the engine directory manually - or you can hardcode the directory here.
  set(UNREAL_ENGINE_DIR $ENV{UE427})
  list(APPEND CESIUM_EXTRA_INCLUDES "${UNREAL_ENGINE_DIR}/Engine/Source/Runtime/TraceLog/Public")
  list(APPEND CESIUM_EXTRA_INCLUDES "${UNREAL_ENGINE_DIR}/Engine/Source/Runtime/Core/Public")
  # Change this line depending on where your project's PCH is.
  list(APPEND CESIUM_EXTRA_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../Intermediate/Build/Win64/UnrealEditor/Development/CesiumRuntime")
  # Let Cesium Native know that we are tracing and overriding the default tracing macros.
  add_compile_definitions(CESIUM_TRACING_ENABLED)
  add_compile_definitions(CESIUM_OVERRIDE_TRACING)

  # Overrides Cesium Native's macro definitions with Unreal Insights macros.
  # This allows us to trace Unreal, Cesium for Unreal, and Cesium Native all in Unreal Insights.
  list(APPEND CESIUM_EXTRA_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()

# vcpkg setup
set(ENV{VCPKG_OVERLAY_PORTS} "${CMAKE_CURRENT_LIST_DIR}/vcpkg-overlays")
set(ENV{VCPKG_OVERLAY_TRIPLETS} "${CMAKE_CURRENT_LIST_DIR}/vcpkg-overlays/triplets")

include(${CMAKE_CURRENT_SOURCE_DIR}/cesium-native/cmake/detect-vcpkg-triplet.cmake)

if (NOT VCPKG_TRIPLET)
    if (DEFINED ENV{VCPKG_TRIPLET})
        set(VCPKG_TRIPLET "$ENV{VCPKG_TRIPLET}")
    elseif(DETECTED_VCPKG_TRIPLET_ERROR)
        message(FATAL_ERROR "${DETECTED_VCPKG_TRIPLET_ERROR}")
    else()
        set(VCPKG_TRIPLET "${DETECTED_VCPKG_TRIPLET}-unreal")
    endif()
endif()

# Our OpenSSL is part of Unreal Engine, so need to install it.
set(CESIUM_EXCLUDE_INSTALL_HEADERS openssl)
set(CESIUM_EXCLUDE_INSTALL_STATIC_LIBS openssl)
# Skip including curl asset accessor as it causes issues on Android and iOS
set(CESIUM_DISABLE_CURL TRUE)

add_subdirectory(cesium-native)

# cesium-native doesn't require this header to be public, but Cesium for Unreal wants to use it.
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cesium-native/extern/stb/stb_image_resize.h TYPE INCLUDE)

if (ANDROID OR IOS)
    # Unreal Engine doesn't include MikkTSpace on Android or iOS.
    # So add our own.
    add_library(MikkTSpace MikkTSpace/mikktspace.c)
    set_target_properties(MikkTSpace PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/MikkTSpace/mikktspace.h")
    target_include_directories(MikkTSpace PUBLIC MikkTSpace)
    install(TARGETS MikkTSpace
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mikktspace
    )
endif()

option(BUILD_SHARED_LIB OFF)
option(SUPPORT_CONSOLE_APP OFF)
set(LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
set(CMAKE_POLICY_VERSION_MINIMUM "3.5")
add_subdirectory(tidy-html5)

# Don't build the cesium-native tests by default
set_target_properties(cesium-native-tests PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)

add_subdirectory(swl-variant)
install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/swl-variant/include/" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
