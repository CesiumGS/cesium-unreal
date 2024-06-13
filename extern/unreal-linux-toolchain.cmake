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
SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -nostdinc++ -I$ENV{UNREAL_ENGINE_LIBCXX_DIR}/include -I$ENV{UNREAL_ENGINE_LIBCXX_DIR}/include/c++/v1 -target x86_64-unknown-linux-gnu")
SET(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld -target x86_64-unknown-linux-gnu --sysroot=$ENV{UNREAL_ENGINE_COMPILER_DIR} -B$ENV{UNREAL_ENGINE_COMPILER_DIR}/usr/lib -B$ENV{UNREAL_ENGINE_COMPILER_DIR}/usr/lib64 -L$ENV{UNREAL_ENGINE_COMPILER_DIR}/usr/lib -L$ENV{UNREAL_ENGINE_COMPILER_DIR}/usr/lib64 -nodefaultlibs -L$ENV{UNREAL_ENGINE_LIBCXX_DIR}/lib/Unix/x86_64-unknown-linux-gnu/ $ENV{UNREAL_ENGINE_LIBCXX_DIR}/lib/Unix/x86_64-unknown-linux-gnu/libc++.a $ENV{UNREAL_ENGINE_LIBCXX_DIR}/lib/Unix/x86_64-unknown-linux-gnu/libc++abi.a -lm -lc -lpthread -lgcc_s -lgcc")

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
