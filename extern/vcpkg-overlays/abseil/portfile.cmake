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
#
# BACKGROUND — Unreal Engine and the Android NDK:
#   Unreal Engine uses C++20. However, the Android NDK version bundled with
#   Unreal has incomplete C++20 *library* support even though the compiler
#   supports C++20 *language* features. This means headers that are compiled
#   as part of Unreal's Android build (including installed vcpkg headers) must
#   not rely on C++20 standard library types such as std::partial_ordering.
#   We build abseil itself with C++17 to avoid any C++20 library dependencies
#   in abseil's own compilation. We additionally patch the installed headers
#   to be safe when included by Unreal's C++20 Android build.

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
        # which prevents abseil from advertising cxx_std_20 as a requirement
        # to consumers (via target_compile_features) even when a C++20 compiler
        # is in use. We intentionally omit it because we set
        # ABSL_PROPAGATE_CXX_STD=OFF below, which makes abseil not propagate
        # any C++ standard requirement at all.
)

# -----------------------------------------------------------------------------
# CUSTOMIZATION 1: Prevent abseil from overriding CMAKE_MSVC_RUNTIME_LIBRARY
# -----------------------------------------------------------------------------
# Abseil's CMakeLists.txt unconditionally sets CMAKE_MSVC_RUNTIME_LIBRARY,
# which conflicts with Unreal Engine's choice of MultiThreadedDLL. We comment
# it out so our setting (established by the triplet) wins.
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
# CUSTOMIZATION 4: Disable std::ordering alias (requires C++20 library)
# -----------------------------------------------------------------------------
# ABSL_OPTION_USE_STD_ORDERING defaults to 2 (auto-detect), which enables the
# alias to std::partial_ordering / std::strong_ordering / std::weak_ordering
# when the compiler reports C++20 support. The Android NDK bundled with Unreal
# Engine supports C++20 language features but has incomplete C++20 *library*
# support, so std::ordering types may be unavailable. We force this to 0 to
# always use abseil's own ordering implementation.
vcpkg_replace_string("${SOURCE_PATH}/absl/base/options.h"
    "#define ABSL_OPTION_USE_STD_ORDERING 2"
    "#define ABSL_OPTION_USE_STD_ORDERING 0")

# -----------------------------------------------------------------------------
# CUSTOMIZATION 5: Fix C++20 three-way comparison check in cord.h (NDK r25)
# -----------------------------------------------------------------------------
# Android NDK r25 defines __cpp_impl_three_way_comparison (indicating compiler
# support for C++20 three-way comparison syntax) but does not define
# __cpp_lib_three_way_comparison (indicating standard library support). Abseil's
# cord.h guards C++20 comparison code with __cpp_impl_three_way_comparison,
# which causes a build failure on NDK r25 when the library support is missing.
# We replace the guard with the library feature macro, which is the correct
# check. See: https://github.com/abseil/abseil-cpp/pull/1728
# NOTE: This patch has already been applied upstream to time.h; only cord.h
# still requires it as of abseil 20260107.1.
vcpkg_replace_string("${SOURCE_PATH}/absl/strings/cord.h"
    "__cpp_impl_three_way_comparison"
    "__cpp_lib_three_way_comparison")

set(ABSL_STATIC_RUNTIME_OPTION "")
if(VCPKG_TARGET_IS_WINDOWS AND VCPKG_CRT_LINKAGE STREQUAL "static")
    set(ABSL_STATIC_RUNTIME_OPTION "-DABSL_MSVC_STATIC_RUNTIME=ON")
endif()

set(ABSL_MINGW_OPTIONS "")
if(VCPKG_TARGET_IS_MINGW)
    # LIBRT-NOTFOUND is needed since the system librt may be found by cmake in
    # a cross-compile setup.
    # See https://github.com/pywinrt/pywinrt/pull/83 for the FIReference
    # definition issue.
    set(ABSL_MINGW_OPTIONS "-DLIBRT=LIBRT-NOTFOUND"
        "-DCMAKE_CXX_FLAGS=-D____FIReference_1_boolean_INTERFACE_DEFINED__")
    # Specify ABSL_BUILD_MONOLITHIC_SHARED_LIBS=ON when VCPKG_LIBRARY_LINKAGE is dynamic to match Abseil's Windows (MSVC) defaults
    if(VCPKG_LIBRARY_LINKAGE STREQUAL dynamic)
        vcpkg_list(APPEND ABSL_MINGW_OPTIONS "-DABSL_BUILD_MONOLITHIC_SHARED_LIBS=ON")
    endif()
endif()
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        # Build abseil with C++17 rather than the upstream default of ON
        # (propagate whatever standard the consumer uses). Unreal Engine uses
        # C++20, but its Android NDK has incomplete C++20 library support.
        # Building with C++17 avoids C++20 library dependencies in abseil's
        # own compiled code. ABSL_PROPAGATE_CXX_STD=OFF prevents abseil from
        # propagating a C++ standard requirement to consumers via
        # target_compile_features, since Unreal controls its own standard.
        -DABSL_PROPAGATE_CXX_STD=OFF
        -DCMAKE_CXX_STANDARD=17
        -DABSL_BUILD_TESTING=OFF
        -DABSL_BUILD_TEST_HELPERS=OFF
        ${ABSL_TEST_HELPERS_OPTIONS}
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
