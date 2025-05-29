include("${CMAKE_CURRENT_LIST_DIR}/shared/common.cmake")

set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME iOS)
set(VCPKG_OSX_DEPLOYMENT_TARGET 15)

# From Unreal Build Tool:
# https://github.com/EpicGames/UnrealEngine/blob/5.3.2-release/Engine/Source/Programs/UnrealBuildTool/Platform/IOS/IOSToolChain.cs
set(VCPKG_CXX_FLAGS "-fvisibility=hidden")
set(VCPKG_C_FLAGS "${VCPKG_CXX_FLAGS}")
