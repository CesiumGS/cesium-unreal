SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR x86_64)

SET(CMAKE_SYSROOT "$ENV{UNREAL_ENGINE_COMPILER_DIR}")

SET(CMAKE_C_COMPILER $ENV{UNREAL_ENGINE_COMPILER_DIR}/bin/clang)
SET(CMAKE_CXX_COMPILER $ENV{UNREAL_ENGINE_COMPILER_DIR}/bin/clang++)
SET(CMAKE_AR $ENV{UNREAL_ENGINE_COMPILER_DIR}/bin/llvm-ar)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH on)
SET(CMAKE_POSITION_INDEPENDENT_CODE on)

# These were deduced by scouring Unreal's LinuxToolChain.cs.
SET(CMAKE_CXX_FLAGS "-nostdinc++ -I$ENV{UNREAL_ENGINE_LIBCXX_DIR}/include -I$ENV{UNREAL_ENGINE_LIBCXX_DIR}/include/c++/v1 -target x86_64-unknown-linux-gnu")
SET(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld -target x86_64-unknown-linux-gnu --sysroot=$ENV{UNREAL_ENGINE_COMPILER_DIR} -B$ENV{UNREAL_ENGINE_COMPILER_DIR}/usr/lib -B$ENV{UNREAL_ENGINE_COMPILER_DIR}/usr/lib64 -L$ENV{UNREAL_ENGINE_COMPILER_DIR}/usr/lib -L$ENV{UNREAL_ENGINE_COMPILER_DIR}/usr/lib64 -nodefaultlibs -L$ENV{UNREAL_ENGINE_LIBCXX_DIR}/lib/Unix/x86_64-unknown-linux-gnu/ $ENV{UNREAL_ENGINE_LIBCXX_DIR}/lib/Unix/x86_64-unknown-linux-gnu/libc++.a $ENV{UNREAL_ENGINE_LIBCXX_DIR}/lib/Unix/x86_64-unknown-linux-gnu/libc++abi.a -lm -lc -lpthread -lgcc_s -lgcc")

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

SET(UNREAL_ROOT_DIR_RELATIVE "$ENV{UNREAL_ENGINE_LIBCXX_DIR}/../../../../../")
FILE(REAL_PATH "${UNREAL_ROOT_DIR_RELATIVE}" UNREAL_ROOT_DIR)

# Some old versions of UE used to have multiple versions of OpenSSL, with different versions used on different platforms.
# That is no longer the case in UE 5.2, 5.3, and 5.4, but the below logic may need to change if that ever happens again.
SET(OPENSSL_VERSIONS_DIR "${UNREAL_ROOT_DIR}/Engine/Source/ThirdParty/OpenSSL")
FILE(GLOB OPENSSL_POSSIBLE_ROOT_DIRS LIST_DIRECTORIES true "${OPENSSL_VERSIONS_DIR}/*")

# Find the subdirectories, each of which should represent an OpenSSL version.
SET(OPENSSL_VERSION_DIRS "")
FOREACH(OPENSSL_POSSIBLE_ROOT_DIR IN LISTS OPENSSL_POSSIBLE_ROOT_DIRS)
  IF(IS_DIRECTORY "${OPENSSL_POSSIBLE_ROOT_DIR}")
    LIST(APPEND OPENSSL_VERSION_DIRS "${OPENSSL_POSSIBLE_ROOT_DIR}")
  ENDIF()
ENDFOREACH()

# There should be exactly one version, otherwise we don't know what to do.
LIST(LENGTH OPENSSL_VERSION_DIRS OPENSSL_VERSION_DIRS_LENGTH)
IF(NOT OPENSSL_VERSION_DIRS_LENGTH EQUAL 1)
  MESSAGE(FATAL_ERROR "Could not deduce the OpenSSL root directory because there is not exactly one directory entry matching `${OPENSSL_VERSIONS_DIR}/1.1.1?`.")
ENDIF()

LIST(GET OPENSSL_VERSION_DIRS 0 OPENSSL_ROOT_DIR)

set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include/Unix/x86_64-unknown-linux-gnu")
set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_ROOT_DIR}/lib/Unix/x86_64-unknown-linux-gnu/libcrypto.a")
set(OPENSSL_SSL_LIBRARY "${OPENSSL_ROOT_DIR}/lib/Unix/x86_64-unknown-linux-gnu/libssl.a")
