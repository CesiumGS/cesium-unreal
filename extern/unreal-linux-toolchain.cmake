SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR x86_64)

SET(CMAKE_SYSROOT "$ENV{UNREAL_ENGINE_COMPILER_DIR}")

SET(CMAKE_C_COMPILER $ENV{UNREAL_ENGINE_COMPILER_DIR}/bin/clang)
SET(CMAKE_CXX_COMPILER $ENV{UNREAL_ENGINE_COMPILER_DIR}/bin/clang++)
SET(CMAKE_AR $ENV{UNREAL_ENGINE_COMPILER_DIR}/bin/llvm-ar)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH on)
SET(CMAKE_POSITION_INDEPENDENT_CODE on)

# Before Unreal Engine 5.6, libcxx was found in a weird place. In 5.6, it's found in that same place,
# plus also under the regular sysroot. In 5.7, it's only available under the regular sysroot.
# Rather than try to figure out what version of UE we're targeting, just always add the legacy location
# resolution after the modern one.
set(LEGACY_LIBCXX_ROOT "$ENV{UNREAL_ENGINE_ROOT}/Engine/Source/ThirdParty/Unix/LibCxx")

# These were deduced by scouring Unreal's LinuxToolChain.cs.
SET(CMAKE_C_FLAGS "-fvisibility-ms-compat -fvisibility-inlines-hidden")
SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -nostdinc++ -I${CMAKE_SYSROOT}/include -I${CMAKE_SYSROOT}/include/c++/v1 -I${LEGACY_LIBCXX_ROOT}/include -I${LEGACY_LIBCXX_ROOT}/include/c++/v1 -target x86_64-unknown-linux-gnu")
SET(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld -target x86_64-unknown-linux-gnu --sysroot=${CMAKE_SYSROOT} -B${CMAKE_SYSROOT}/usr/lib -B${CMAKE_SYSROOT}/usr/lib64 -L${CMAKE_SYSROOT}/usr/lib -L${CMAKE_SYSROOT}/usr/lib64 -L${LEGACY_LIBCXX_ROOT}/lib/Unix/x86_64-unknown-linux-gnu/ -nodefaultlibs -lc++ -lc++abi -lm -lc -lpthread -lgcc_s -lgcc")

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
SET(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
