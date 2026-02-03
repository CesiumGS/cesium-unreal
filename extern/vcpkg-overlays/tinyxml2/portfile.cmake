# This port uses tinyxml2 from Unreal Engine, instead of the normal vcpkg version.
# This is important for avoiding linker errors (multiply defined symbols) when
# building and packaging Unreal Engine games that include cesium-native.
#
# The environment variable `UNREAL_ENGINE_ROOT` must be set to the root of the
# Unreal Engine installation to use. It should use forward slashes even on Windows
# and it should _not_ end with a slash.
# For example: `C:/Program Files/Epic Games/UE_5.3`

# This was copied from the openssl vcpkg overlay. Unfortunately,
# directory structure of Unreal's third party tree is not consistent
# across packages, especially with respect to include file
# directories, so we have to repeat most of the code here, slightly
# tweaked.

set(VCPKG_POLICY_EMPTY_PACKAGE enabled)

if (NOT DEFINED ENV{UNREAL_ENGINE_ROOT})
  message(FATAL_ERROR "The environment variable `UNREAL_ENGINE_ROOT` must be defined.")
endif()

message(STATUS "Using Unreal Engine installation at $ENV{UNREAL_ENGINE_ROOT}")

set(TINYXML2_VERSIONS_DIR "$ENV{UNREAL_ENGINE_ROOT}/Engine/Source/ThirdParty/TinyXML2")
file(GLOB TINYXML2_POSSIBLE_ROOT_DIRS LIST_DIRECTORIES true "${TINYXML2_VERSIONS_DIR}/*")

# Find the subdirectories, each of which should represent an TinyXML2 version.
set(TINYXML2_VERSION_DIRS "")
foreach(TINYXML2_POSSIBLE_ROOT_DIR IN LISTS TINYXML2_POSSIBLE_ROOT_DIRS)
  if(IS_DIRECTORY "${TINYXML2_POSSIBLE_ROOT_DIR}")
    list(APPEND TINYXML2_VERSION_DIRS "${TINYXML2_POSSIBLE_ROOT_DIR}")
  endif()
endforeach()

# There should be exactly one version, otherwise we don't know what to do.
list(LENGTH TINYXML2_VERSION_DIRS TINYXML2_VERSION_DIRS_LENGTH)
if(NOT TINYXML2_VERSION_DIRS_LENGTH EQUAL 1)
  message(FATAL_ERROR "Could not deduce the TinyXML2 root directory because there is not exactly one directory matching `${TINYXML2_VERSIONS_DIR}/*`.")
endif()

list(GET TINYXML2_VERSION_DIRS 0 TINYXML2_ROOT_DIR)

if (VCPKG_TARGET_IS_ANDROID)
  set(TINYXML2_INCLUDE_PATH "${TINYXML2_ROOT_DIR}/include/Android")
  if (VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(TINYXML2_LIB_PATH "${TINYXML2_ROOT_DIR}/lib/Android/x64")
  elseif (VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
    set(TINYXML2_LIB_PATH "${TINYXML2_ROOT_DIR}/lib/Android/ARM64")
  else()
    message(FATAL_ERROR "Unknown Unreal / TinyXML2 paths for Android platform with architecture ${VCPKG_TARGET_ARCHITECTURE}.")
  endif()
elseif (VCPKG_TARGET_IS_IOS)
  set(TINYXML2_LIB_PATH "${TINYXML2_ROOT_DIR}/lib/IOS")
  set(TINYXM2_LIB_LOCATION "${TINYXML2_LIB_PATH}/libtinyxml2.a")
  set(TINYXML2_INCLUDE_PATH "${TINYXML2_ROOT_DIR}/include")
elseif (VCPKG_TARGET_IS_OSX)
  set(TINYXML2_LIB_PATH "${TINYXML2_ROOT_DIR}/lib/Mac")
  set(TINYXM2_LIB_LOCATION "${TINYXML2_LIB_PATH}/libtinyxml2.a")
  set(TINYXML2_INCLUDE_PATH "${TINYXML2_ROOT_DIR}/include")
elseif (VCPKG_TARGET_IS_WINDOWS)
  set(TINYXML2_INCLUDE_PATH "${TINYXML2_ROOT_DIR}/include")
  set(TINYXML2_LIB_PATH "${TINYXML2_ROOT_DIR}/lib/Win64/Release")
  set(TINYXML2_LIB_LOCATION "${TINYXML2_LIB_PATH}/tinyxml2.lib")
elseif (VCPKG_TARGET_IS_LINUX)
  if (VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(TINYXML2_PLATFORM "x86_64-unknown-linux-gnu")
  elseif (VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
    set(TINYXML2_PLATFORM "aarch64-unknown-linux-gnueabi")
  else()
    message(FATAL_ERROR "Unknown Unreal / TinyXML2 paths for Linux platform with architecture ${VCPKG_TARGET_ARCHITECTURE}.")
  endif()

  set(TINYXML2_LIB_PATH "${TINYXML2_ROOT_DIR}/lib/Unix/${TINYXML2_PLATFORM}")
  set(TINYXM2_LIB_LOCATION "${TINYXML2_LIB_PATH}/libtinyxml2.a")
  set(TINYXML2_INCLUDE_PATH "${TINYXML2_ROOT_DIR}/include")
else()
  message(FATAL_ERROR "Unknown Unreal / TinyXML2 paths for VCPKG_CMAKE_SYSTEM_NAME ${VCPKG_CMAKE_SYSTEM_NAME}.")
endif()

configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/Tinyxml2Config.cmake.in"
  "${CURRENT_PACKAGES_DIR}/share/tinyxml2/tinyxml2Config.cmake"
  @ONLY)
