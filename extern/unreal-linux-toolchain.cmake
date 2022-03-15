SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR x86_64)

file(TO_CMAKE_PATH "$ENV{LINUX_MULTIARCH_ROOT}/x86_64-unknown-linux-gnu" UNREAL_ENGINE_COMPILER_DIR)

SET(CMAKE_SYSROOT ${UNREAL_ENGINE_COMPILER_DIR})

SET(CMAKE_C_COMPILER "${UNREAL_ENGINE_COMPILER_DIR}/bin/clang")
SET(CMAKE_CXX_COMPILER "${UNREAL_ENGINE_COMPILER_DIR}/bin/clang++")

if(CMAKE_HOST_WIN32)
  SET(CMAKE_C_COMPILER "${CMAKE_C_COMPILER}.exe")
  SET(CMAKE_CXX_COMPILER "${CMAKE_CXX_COMPILER}.exe")
endif()

set(CMAKE_C_COMPILER_TARGET x86_64-unknown-linux-gnu)
set(CMAKE_CXX_COMPILER_TARGET x86_64-unknown-linux-gnu)
string(APPEND CMAKE_CXX_FLAGS_INIT "\"-isystem$ENV{UNREAL_ENGINE_DIR}/Engine/Source/ThirdParty/Linux/LibCxx/include\" \"-isystem$ENV{UNREAL_ENGINE_DIR}/Engine/Source/ThirdParty/Linux/LibCxx/include/c++/v1\" -target x86_64-unknown-linux-gnu -Qunused-arguments -pthread ")
string(APPEND CMAKE_C_FLAGS_INIT "-target x86_64-unknown-linux-gnu -pthread ")
string(APPEND CMAKE_EXE_LINKER_FLAGS_INIT "\"-L$ENV{UNREAL_ENGINE_DIR}/Engine/Source/ThirdParty/Linux/LibCxx/lib/Linux/x86_64-unknown-linux-gnu\" -pthread ")

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
