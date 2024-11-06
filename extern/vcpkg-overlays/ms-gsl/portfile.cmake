if (NOT DEFINED ENV{UNREAL_ENGINE_ROOT})
  message(FATAL_ERROR "The environment variable `UNREAL_ENGINE_ROOT` must be defined.")
endif()

message(STATUS "Using Unreal Engine installation at $ENV{UNREAL_ENGINE_ROOT}")

# UE 5.5 is the first version of UE to include GSL, and it only has a single version of it. This logic may need to
# change if a future UE version includes multiple copies of GSL.
set(GSL_VERSIONS_DIR "$ENV{UNREAL_ENGINE_ROOT}/Engine/Source/ThirdParty/GuidelinesSupportLibrary")
file(GLOB GSL_POSSIBLE_ROOT_DIRS LIST_DIRECTORIES true "${GSL_VERSIONS_DIR}/*")

# Find the subdirectories, each of which should represent a GSL version.
set(GSL_VERSION_DIRS "")
foreach(GSL_POSSIBLE_ROOT_DIR IN LISTS GSL_POSSIBLE_ROOT_DIRS)
  if(IS_DIRECTORY "${GSL_POSSIBLE_ROOT_DIR}")
    list(APPEND GSL_VERSION_DIRS "${GSL_POSSIBLE_ROOT_DIR}")
  endif()
endforeach()

# There should be either zero versions (UE 5.2-5.4) or one version (UE 5.5).
# Otherwise we don't know what to do.
list(LENGTH GSL_VERSION_DIRS GSL_VERSION_DIRS_LENGTH)
if(GSL_VERSION_DIRS_LENGTH EQUAL 0)
  # Use the standard GSL from vcpkg

  #header-only library with an install target
  vcpkg_from_github(
      OUT_SOURCE_PATH SOURCE_PATH
      REPO Microsoft/GSL
      REF a3534567187d2edc428efd3f13466ff75fe5805c
      SHA512 5bd6aad37fee3b56a2ee2fed10d6ef02fdcf37a1f40b3fb1bbec8146a573e235169b315405d010ab75175674ed82658c8202f40b128a849c5250b4a1b8b0a1b3
      HEAD_REF main
  )

  vcpkg_cmake_configure(
      SOURCE_PATH ${SOURCE_PATH}
      OPTIONS
          -DGSL_TEST=OFF
  )

  vcpkg_cmake_install()

  vcpkg_cmake_config_fixup(
      PACKAGE_NAME Microsoft.GSL
      CONFIG_PATH share/cmake/Microsoft.GSL
  )

  file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")

  # Handle copyright
  file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
elseif(NOT GSL_VERSION_DIRS_LENGTH EQUAL 1)
  message(FATAL_ERROR "Could not deduce the GSL root directory because there is not exactly one directory matching `${GSL_VERSIONS_DIR}/*`.")
else()
  # Use Unreal Engine's copy of GSL
  list(GET GSL_VERSION_DIRS 0 GSL_ROOT_DIR)

  set(SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}/ue55")
  message("***** ${SOURCE_PATH}")

  vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
      "-DUNREAL_GSL=${GSL_ROOT_DIR}"
  )

  vcpkg_cmake_install()

  vcpkg_cmake_config_fixup(
      PACKAGE_NAME Microsoft.GSL
      CONFIG_PATH share/cmake/Microsoft.GSL
  )
endif()
