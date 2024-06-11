set(VCPKG_POLICY_EMPTY_PACKAGE enabled)

message(STATUS "Using Unreal Engine installation at $ENV{UNREAL_ENGINE_ROOT}")

# Some old versions of UE used to have multiple versions of OpenSSL, with different versions used on different platforms.
# That is no longer the case in UE 5.2, 5.3, and 5.4, but the below logic may need to change if that ever happens again.
set(OPENSSL_VERSIONS_DIR "$ENV{UNREAL_ENGINE_ROOT}/Engine/Source/ThirdParty/OpenSSL")
file(GLOB OPENSSL_POSSIBLE_ROOT_DIRS LIST_DIRECTORIES true "${OPENSSL_VERSIONS_DIR}/*")

# Find the subdirectories, each of which should represent an OpenSSL version.
set(OPENSSL_VERSION_DIRS "")
foreach(OPENSSL_POSSIBLE_ROOT_DIR IN LISTS OPENSSL_POSSIBLE_ROOT_DIRS)
  if(IS_DIRECTORY "${OPENSSL_POSSIBLE_ROOT_DIR}")
    list(APPEND OPENSSL_VERSION_DIRS "${OPENSSL_POSSIBLE_ROOT_DIR}")
  endif()
endforeach()

# There should be exactly one version, otherwise we don't know what to do.
list(LENGTH OPENSSL_VERSION_DIRS OPENSSL_VERSION_DIRS_LENGTH)
if(NOT OPENSSL_VERSION_DIRS_LENGTH EQUAL 1)
  message(FATAL_ERROR "Could not deduce the OpenSSL root directory because there is not exactly one directory matching `${OPENSSL_VERSIONS_DIR}/*`.")
endif()

list(GET OPENSSL_VERSION_DIRS 0 OPENSSL_ROOT_DIR)

if (VCPKG_TARGET_IS_ANDROID)
elseif (VCPKG_TARGET_IS_IOS)
elseif (VCPKG_TARGET_IS_OSX)
elseif (VCPKG_TARGET_IS_WINDOWS)
elseif (VCPKG_TARGET_IS_LINUX)
  if (VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(OPENSSL_LIB_PATH "${OPENSSL_ROOT_DIR}/lib/Unix/x86_64-unknown-linux-gnu")
    set(OPENSSL_INCLUDE_PATH "${OPENSSL_ROOT_DIR}/include/Unix/x86_64-unknown-linux-gnu")
  elseif (VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
    set(OPENSSL_LIB_PATH "${OPENSSL_ROOT_DIR}/lib/Unix/aarch64-unknown-linux-gnueabi")
    set(OPENSSL_INCLUDE_PATH "${OPENSSL_ROOT_DIR}/include/Unix/aarch64-unknown-linux-gnueabi")
  else()
    message(FATAL_ERROR "Unknown Unreal / OpenSSL paths for Linux platform with architecture ${VCPKG_TARGET_ARCHITECTURE}.")
  endif()
else()
  message(FATAL_ERROR "Unknown Unreal / OpenSSL paths for VCPKG_CMAKE_SYSTEM_NAME ${VCPKG_CMAKE_SYSTEM_NAME}.")
endif()

file(INSTALL "${OPENSSL_LIB_PATH}/" DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
file(INSTALL "${OPENSSL_INCLUDE_PATH}/" DESTINATION "${CURRENT_PACKAGES_DIR}/include")

configure_file("${CMAKE_CURRENT_LIST_DIR}/libcrypto.pc.in" "${CURRENT_PACKAGES_DIR}/lib/pkgconfig/libcrypto.pc" @ONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/libssl.pc.in" "${CURRENT_PACKAGES_DIR}/lib/pkgconfig/libssl.pc" @ONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/openssl.pc.in" "${CURRENT_PACKAGES_DIR}/lib/pkgconfig/openssl.pc" @ONLY)
