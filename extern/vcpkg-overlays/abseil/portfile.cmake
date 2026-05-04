# =============================================================================
# Cesium for Unreal — Custom Abseil overlay port
# =============================================================================
#
# This overlay customizes the standard vcpkg abseil port in several ways
# required for compatibility with Unreal Engine. When upgrading the abseil
# version, re-apply each customization described below and verify it is still
# necessary.
#
# CUSTOMIZATION HISTORY:
#   Originally created Nov 2024 (abseil 20240722.0) to fix Android and MSVC
#   build issues. Updated to 20260107.1 to match the vcpkg 2026.04.27 baseline.

if(NOT VCPKG_TARGET_IS_WINDOWS)
    vcpkg_check_linkage(ONLY_STATIC_LIBRARY)
endif()

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO abseil/abseil-cpp
    REF "20260107.1"
    SHA512 f5012885d6b6844a9cf5ed92ad5468b8757db33dfe1364bfb232fff928e06c550c7eb4557f45186a8ac4d18b178df9be267681abab4a6de40823b574afbe9960
    HEAD_REF master
    PATCHES
        # Fixes a missing TESTONLY marker that causes build errors when tests
        # are not being built. Taken from the upstream vcpkg port.
        fix-heterogeneous_lookup_testing-target.patch
        # Fixes DLL builds when targeting MinGW. Taken from the upstream vcpkg
        # port.
        fix-mingw-dll.patch
        # NOTE: The upstream vcpkg port also includes 003-force-cxx-17.patch,
        # which forces abseil to advertise C++17 language features even when
        # the compiler is in C++14 mode. We intentionally omit that patch
        # because Unreal Engine is built with C++14, and we must match.
)

# -----------------------------------------------------------------------------
# CUSTOMIZATION 1: Prevent abseil from overriding CMAKE_MSVC_RUNTIME_LIBRARY
# -----------------------------------------------------------------------------
# Abseil's CMakeLists.txt unconditionally sets CMAKE_MSVC_RUNTIME_LIBRARY,
# which conflicts with Unreal Engine's choice of MultiThreadedDLL. We comment
# it out so our setting (established before this subdirectory is added) wins.
vcpkg_replace_string("${SOURCE_PATH}/CMakeLists.txt"
    "set(CMAKE_MSVC_RUNTIME_LIBRARY \"MultiThreaded$<$<CONFIG:Debug>:Debug>\")"
    "#set(CMAKE_MSVC_RUNTIME_LIBRARY \"MultiThreaded$<$<CONFIG:Debug>:Debug>\")")
vcpkg_replace_string("${SOURCE_PATH}/CMakeLists.txt"
    "set(CMAKE_MSVC_RUNTIME_LIBRARY \"MultiThreaded$<$<CONFIG:Debug>:Debug>DLL\")"
    "#set(CMAKE_MSVC_RUNTIME_LIBRARY \"MultiThreaded$<$<CONFIG:Debug>:Debug>DLL\")")

# -----------------------------------------------------------------------------
# CUSTOMIZATION 2: Rename the inline namespace
# -----------------------------------------------------------------------------
# Unreal Engine ships its own copy of abseil. To prevent ODR violations and
# linker errors when both copies are linked into the same binary, we rename
# abseil's inline namespace from "lts_20260107" to
# "lts_20260107_cesium_for_unreal". This makes all mangled symbol names unique.
vcpkg_replace_string("${SOURCE_PATH}/absl/base/options.h"
    "ABSL_OPTION_INLINE_NAMESPACE_NAME lts_20260107"
    "ABSL_OPTION_INLINE_NAMESPACE_NAME lts_20260107_cesium_for_unreal")

# -----------------------------------------------------------------------------
# CUSTOMIZATION 3: Suppress ABSL_LTS_RELEASE_VERSION
# -----------------------------------------------------------------------------
# Commenting out ABSL_LTS_RELEASE_VERSION and ABSL_LTS_RELEASE_PATCH_LEVEL
# prevents code that checks those macros from treating our modified build as
# the canonical abseil LTS release. This avoids version-check mismatches when
# our customized abseil is used alongside Unreal's bundled abseil.
vcpkg_replace_string("${SOURCE_PATH}/absl/base/config.h"
    "#define ABSL_LTS_RELEASE_VERSION 20260107"
    "//#define ABSL_LTS_RELEASE_VERSION 20260107")
vcpkg_replace_string("${SOURCE_PATH}/absl/base/config.h"
    "#define ABSL_LTS_RELEASE_PATCH_LEVEL 1"
    "//#define ABSL_LTS_RELEASE_PATCH_LEVEL 1")

# -----------------------------------------------------------------------------
# CUSTOMIZATION 4: Disable std::ordering alias (requires C++20)
# -----------------------------------------------------------------------------
# ABSL_OPTION_USE_STD_ORDERING defaults to 2 (auto-detect), which would enable
# the alias when compiling in C++20 mode. Unreal Engine uses C++14, so we force
# this to 0 to always use abseil's own implementation and avoid any accidental
# dependency on std::ordering.
vcpkg_replace_string("${SOURCE_PATH}/absl/base/options.h"
    "#define ABSL_OPTION_USE_STD_ORDERING 2"
    "#define ABSL_OPTION_USE_STD_ORDERING 0")

set(ABSL_STATIC_RUNTIME_OPTION "")
if(VCPKG_TARGET_IS_WINDOWS AND VCPKG_CRT_LINKAGE STREQUAL "static")
    set(ABSL_STATIC_RUNTIME_OPTION "-DABSL_MSVC_STATIC_RUNTIME=ON")
endif()

set(ABSL_MINGW_OPTIONS "")
if(VCPKG_TARGET_IS_MINGW)
    set(ABSL_MINGW_OPTIONS "-DLIBRT=LIBRT-NOTFOUND"
        "-DCMAKE_CXX_FLAGS=-D____FIReference_1_boolean_INTERFACE_DEFINED__")
    if(VCPKG_LIBRARY_LINKAGE STREQUAL dynamic)
        vcpkg_list(APPEND ABSL_MINGW_OPTIONS "-DABSL_BUILD_MONOLITHIC_SHARED_LIBS=ON")
    endif()
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    DISABLE_PARALLEL_CONFIGURE
    OPTIONS
        # CUSTOMIZATION 1 (continued): Force C++14 to match Unreal Engine's ABI.
        # ABSL_PROPAGATE_CXX_STD=OFF prevents abseil from inheriting the
        # compiler's default standard, and CMAKE_CXX_STANDARD=14 pins it
        # explicitly. The upstream port uses ABSL_PROPAGATE_CXX_STD=ON with
        # C++17 forced via patch; we intentionally diverge here.
        -DABSL_PROPAGATE_CXX_STD=OFF
        -DCMAKE_CXX_STANDARD=14
        -DABSL_BUILD_TESTING=OFF
        -DABSL_BUILD_TEST_HELPERS=OFF
        ${ABSL_STATIC_RUNTIME_OPTION}
        ${ABSL_MINGW_OPTIONS}
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME absl CONFIG_PATH lib/cmake/absl)

if(VCPKG_TARGET_IS_IOS OR VCPKG_TARGET_IS_OSX)
    file(APPEND "${CURRENT_PACKAGES_DIR}/lib/pkgconfig/absl_time.pc" "Libs.private: -framework CoreFoundation\n")
    if(NOT VCPKG_BUILD_TYPE)
        file(APPEND "${CURRENT_PACKAGES_DIR}/debug/lib/pkgconfig/absl_time.pc" "Libs.private: -framework CoreFoundation\n")
    endif()
endif()
vcpkg_fixup_pkgconfig()

vcpkg_copy_pdbs()
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share"
                    "${CURRENT_PACKAGES_DIR}/debug/include"
                    "${CURRENT_PACKAGES_DIR}/include/absl/copts"
                    "${CURRENT_PACKAGES_DIR}/include/absl/strings/testdata"
                    "${CURRENT_PACKAGES_DIR}/include/absl/time/internal/cctz/testdata"
)

if(VCPKG_TARGET_IS_WINDOWS AND VCPKG_LIBRARY_LINKAGE STREQUAL "dynamic")
    vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/include/absl/base/config.h" "defined(ABSL_CONSUME_DLL)" "1")
    vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/include/absl/base/internal/thread_identity.h" "defined(ABSL_CONSUME_DLL)" "1")
endif()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
