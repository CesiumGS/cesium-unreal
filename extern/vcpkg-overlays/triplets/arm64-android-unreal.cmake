include("${CMAKE_CURRENT_LIST_DIR}/shared/common.cmake")

set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Android)
set(VCPKG_MAKE_BUILD_TRIPLET "--host=aarch64-linux-android")
set(VCPKG_CMAKE_CONFIGURE_OPTIONS -DANDROID_ABI=arm64-v8a)
set(VCPKG_CMAKE_SYSTEM_VERSION 21)

# From Unreal Build Tool:
# https://github.com/EpicGames/UnrealEngine/blob/5.2.1-release/Engine/Source/Programs/UnrealBuildTool/Platform/Android/AndroidToolChain.cs
set(VCPKG_CXX_FLAGS "-fvisibility=hidden -fvisibility-inlines-hidden")
set(VCPKG_C_FLAGS "${VCPKG_CXX_FLAGS}")
