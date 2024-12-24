include("${CMAKE_CURRENT_LIST_DIR}/shared/common.cmake")

set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_ENV_PASSTHROUGH "UNREAL_ENGINE_ROOT")
set(VCPKG_POLICY_ONLY_RELEASE_CRT enabled)

# Unreal Engine adds /Zp8 in 64-bit Windows builds to align structs to 8 bytes instead of the
# default of 16 bytes. There's this nice note in the documentation for that option:
#   Don't change the setting from the default when you include the Windows SDK headers, either
#   by using /Zp on the command line or by using #pragma pack. Otherwise, your application may
#   cause memory corruption at runtime.
# (https://docs.microsoft.com/en-us/cpp/build/reference/zp-struct-member-alignment?view=msvc-160)
# Yeah that's not just the Windows SDK, but anything that passes structs across the boundary
# between compilation units using different versions of that flag. We compile cesium-native
# with this same option to avoid super-dodgy and hard to debug issues.
# https://github.com/EpicGames/UnrealEngine/blob/5.3.2-release/Engine/Source/Programs/UnrealBuildTool/Platform/Windows/VCToolChain.cs
set(VCPKG_CXX_FLAGS "/Zp8")
set(VCPKG_C_FLAGS "${VCPKG_CXX_FLAGS}")

# Use an unreasonable amount of force to replace /MDd (MultiThreadedDebugDLL) with /MD (MultiThreadedDLL)
# and ensure that `_DEBUG` is not defined, as required by Unreal Engine's use of the release runtime library.
# Only CMAKE_MSVC_RUNTIME_LIBRARY would be required if all our third-party libraries used CMake 3.15+, but alas.
set(VCPKG_CMAKE_CONFIGURE_OPTIONS "")
list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS "-DCMAKE_MSVC_RUNTIME_LIBRARY:STRING=MultiThreadedDLL")
list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS "-DCMAKE_CXX_FLAGS_DEBUG:STRING=/MD /Z7 /Ob0 /Od /RTC1")
list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS "-DCMAKE_C_FLAGS_DEBUG:STRING=/MD /Z7 /Ob0 /Od /RTC1")

# When building official binaries on CI, use a very specific MSVC toolset version (which must be installed).
# When building locally, use the default.
if(DEFINED ENV{CI})
  # Toolset version should be 14.38 on UE 5.5+, 14.34 on prior versions.
  set(VCPKG_PLATFORM_TOOLSET_VERSION "14.34")

  set(UNREAL_ENGINE_BUILD_VERSION_FILENAME "$ENV{UNREAL_ENGINE_ROOT}/Engine/Build/Build.version")
  if(EXISTS "${UNREAL_ENGINE_BUILD_VERSION_FILENAME}")
    file(READ "${UNREAL_ENGINE_BUILD_VERSION_FILENAME}" UNREAL_ENGINE_BUILD_VERSION)
    string(JSON UNREAL_MAJOR_VERSION GET "${UNREAL_ENGINE_BUILD_VERSION}" "MajorVersion")
    string(JSON UNREAL_MINOR_VERSION GET "${UNREAL_ENGINE_BUILD_VERSION}" "MinorVersion")
    if("${UNREAL_MAJOR_VERSION}" GREATER "5" OR "${UNREAL_MINOR_VERSION}" GREATER_EQUAL "5")
      # This is UE 5.5+, so use MSVC 14.38.
      set(VCPKG_PLATFORM_TOOLSET_VERSION "14.38")
    endif()
  endif()
endif()
