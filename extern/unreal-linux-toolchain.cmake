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
SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -nostdinc++ -target x86_64-unknown-linux-gnu")
SET(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld -target x86_64-unknown-linux-gnu --sysroot=${CMAKE_SYSROOT} -B${CMAKE_SYSROOT}/usr/lib64 -B${CMAKE_SYSROOT}/usr/lib -L${CMAKE_SYSROOT}/usr/lib64 -L${CMAKE_SYSROOT}/usr/lib -nodefaultlibs -lm -lc -lpthread -lgcc_s -lgcc")

# Before Unreal Engine 5.6, libcxx was found in a weird place. In 5.6, it's found in that same place,
# plus also under the regular sysroot, but trying to use the copy in the sysroot causes problems. In
# 5.7, it's only available under the regular sysroot.
# So figure out which version of UE we're building against, and set up the include/linker paths accordingly.
set(USE_LEGACY_LIBCPP OFF)
set(UNREAL_ENGINE_BUILD_VERSION_FILENAME "$ENV{UNREAL_ENGINE_ROOT}/Engine/Build/Build.version")
if(EXISTS "${UNREAL_ENGINE_BUILD_VERSION_FILENAME}")
  file(READ "${UNREAL_ENGINE_BUILD_VERSION_FILENAME}" UNREAL_ENGINE_BUILD_VERSION)
  string(JSON UNREAL_MAJOR_VERSION GET "${UNREAL_ENGINE_BUILD_VERSION}" "MajorVersion")
  string(JSON UNREAL_MINOR_VERSION GET "${UNREAL_ENGINE_BUILD_VERSION}" "MinorVersion")
  if("${UNREAL_MAJOR_VERSION}" EQUAL "5" AND "${UNREAL_MINOR_VERSION}" LESS_EQUAL "6")
    # This is an older version of UE, so use the legacy location for libcxx.
    set(USE_LEGACY_LIBCPP ON)
  endif()
endif()

if(USE_LEGACY_LIBCPP)
  set(LEGACY_LIBCXX_ROOT "$ENV{UNREAL_ENGINE_ROOT}/Engine/Source/ThirdParty/Unix/LibCxx")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${LEGACY_LIBCXX_ROOT}/include -I${LEGACY_LIBCXX_ROOT}/include/c++/v1")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${LEGACY_LIBCXX_ROOT}/lib/Unix/x86_64-unknown-linux-gnu/ ${LEGACY_LIBCXX_ROOT}/lib/Unix/x86_64-unknown-linux-gnu/libc++.a ${LEGACY_LIBCXX_ROOT}/lib/Unix/x86_64-unknown-linux-gnu/libc++abi.a")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${CMAKE_SYSROOT}/include -I${CMAKE_SYSROOT}/include/c++/v1 ")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc++ -lc++abi")
endif()

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
SET(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
