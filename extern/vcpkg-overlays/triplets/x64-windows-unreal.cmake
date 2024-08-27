include("${CMAKE_CURRENT_LIST_DIR}/shared/common.cmake")

set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_ENV_PASSTHROUGH "UNREAL_ENGINE_ROOT")

# Unreal Engine adds /Zp8 in 64-bit Windows builds to align structs to 8 bytes instead of the
# default of 16 bytes. There's this nice note in the documentation for that option:
#   Don't change the setting from the default when you include the Windows SDK headers, either
#   by using /Zp on the command line or by using #pragma pack. Otherwise, your application may
#   cause memory corruption at runtime.
# (https://docs.microsoft.com/en-us/cpp/build/reference/zp-struct-member-alignment?view=msvc-160)
# Yeah that's not just the Windows SDK, but anything that passes structs across the boundary
# between compilation units using different versions of that flag. We compile cesium-native
# with this same option to avoid super-dodgy and hard to debug issues.
# https://github.com/EpicGames/UnrealEngine/blob/5.2.1-release/Engine/Source/Programs/UnrealBuildTool/Platform/Windows/VCToolChain.cs
set(VCPKG_CXX_FLAGS "/MD /Zp8")
set(VCPKG_C_FLAGS "${VCPKG_CXX_FLAGS}")

# When building official binaries on CI, use a very specific MSVC toolset version (which must be installed).
# When building locally, use the default.
if(DEFINED ENV{CI})
  set(VCPKG_PLATFORM_TOOLSET_VERSION "14.34")
endif()
