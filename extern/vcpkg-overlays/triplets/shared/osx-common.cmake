include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_OSX_DEPLOYMENT_TARGET 10.15)

# From Unreal Build Tool:
# https://github.com/EpicGames/UnrealEngine/blob/5.3.2-release/Engine/Source/Programs/UnrealBuildTool/Platform/Mac/MacToolChain.cs
set(VCPKG_CXX_FLAGS "-fvisibility-ms-compat -fvisibility-inlines-hidden")
set(VCPKG_C_FLAGS "${VCPKG_CXX_FLAGS}")
