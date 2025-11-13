SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR x86_64)

SET(CMAKE_SYSROOT "$ENV{UNREAL_ENGINE_COMPILER_DIR}")

SET(CMAKE_C_COMPILER $ENV{UNREAL_ENGINE_COMPILER_DIR}/bin/clang)
SET(CMAKE_CXX_COMPILER $ENV{UNREAL_ENGINE_COMPILER_DIR}/bin/clang++)
SET(CMAKE_AR $ENV{UNREAL_ENGINE_COMPILER_DIR}/bin/llvm-ar)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH on)
SET(CMAKE_POSITION_INDEPENDENT_CODE on)

# These were deduced by scouring Unreal's LinuxToolChain.cs.
SET(CMAKE_C_FLAGS "-fvisibility-ms-compat -fvisibility-inlines-hidden")
SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -nostdinc++ -I${CMAKE_SYSROOT}/include -I${CMAKE_SYSROOT}/include/c++/v1 -target x86_64-unknown-linux-gnu")
SET(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld -target x86_64-unknown-linux-gnu --sysroot=${CMAKE_SYSROOT} -B${CMAKE_SYSROOT}/usr/lib -B${CMAKE_SYSROOT}/usr/lib64 -L${CMAKE_SYSROOT}/usr/lib -L${CMAKE_SYSROOT}/usr/lib64 -nodefaultlibs ${CMAKE_SYSROOT}/lib64/libc++.a ${CMAKE_SYSROOT}/lib64/libc++abi.a -lm -lc -lpthread -lgcc_s -lgcc")

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
SET(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
