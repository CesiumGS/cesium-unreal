cmake_minimum_required(VERSION 3.15)

project(
    cesium-unreal-extern
    VERSION 0.1.0
    LANGUAGES CXX
)

# uriparser is detecting that `reallocarray` is available, even though libc++ doesn't have it.
# Would be good to figure out why, but for now we're just forcing it to think otherwise.
set(HAVE_REALLOCARRAY OFF)

# Add -fPIC, which (I guess?) is necessary because we're building static libraries that will
# eventually be linked into a shared library.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Don't use the normal C++ standard library, but instead use the one bundled with Unreal Engine.
add_compile_options(-nostdinc++ -isystem /home/kring/unreal/UnrealEngine/Engine/Source/ThirdParty/Linux/LibCxx/include/c++/v1)

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)

add_subdirectory(cesium-native)
