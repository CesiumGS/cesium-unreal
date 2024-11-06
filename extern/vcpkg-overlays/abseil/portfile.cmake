if(NOT VCPKG_TARGET_IS_WINDOWS)
    vcpkg_check_linkage(ONLY_STATIC_LIBRARY)
endif()

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO abseil/abseil-cpp
    REF "20240722.0"
    SHA512 bd2cca8f007f2eee66f51c95a979371622b850ceb2ce3608d00ba826f7c494a1da0fba3c1427728f2c173fe50d59b701da35c2c9fdad2752a5a49746b1c8ef31
    HEAD_REF master
)

# With ABSL_PROPAGATE_CXX_STD=ON abseil automatically detect if it is being
# compiled with C++14 or C++17, and modifies the installed `absl/base/options.h`
# header accordingly. This works even if CMAKE_CXX_STANDARD is not set. Abseil
# uses the compiler default behavior to update `absl/base/options.h` as needed.
set(ABSL_USE_CXX17_OPTION "")
if("cxx17" IN_LIST FEATURES)
    set(ABSL_USE_CXX17_OPTION "-DCMAKE_CXX_STANDARD=17")
endif()

set(ABSL_STATIC_RUNTIME_OPTION "")
if(VCPKG_TARGET_IS_WINDOWS AND VCPKG_CRT_LINKAGE STREQUAL "static")
    set(ABSL_STATIC_RUNTIME_OPTION "-DABSL_MSVC_STATIC_RUNTIME=ON")
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    DISABLE_PARALLEL_CONFIGURE
    OPTIONS
        -DABSL_PROPAGATE_CXX_STD=ON
        ${ABSL_USE_CXX17_OPTION}
        ${ABSL_STATIC_RUNTIME_OPTION}
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME absl CONFIG_PATH lib/cmake/absl)
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

# Don't let our customized version of Abseil pose as the real thing.
vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/include/absl/base/options.h" "ABSL_OPTION_INLINE_NAMESPACE_NAME lts_20240722" "ABSL_OPTION_INLINE_NAMESPACE_NAME lts_20240722_cesium_for_unreal")
vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/include/absl/base/config.h" "#define ABSL_LTS_RELEASE_VERSION 20240722" "//#define ABSL_LTS_RELEASE_VERSION 20240722")
vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/include/absl/base/config.h" "#define ABSL_LTS_RELEASE_PATCH_LEVEL 0" "//#define ABSL_LTS_RELEASE_PATCH_LEVEL 0")

# Apply this patch to fix C++20 build with Android NDK r25
# https://github.com/abseil/abseil-cpp/pull/1728
vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/include/absl/time/time.h" "__cpp_impl_three_way_comparison" "__cpp_lib_three_way_comparison")
vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/include/absl/strings/cord.h" "__cpp_impl_three_way_comparison" "__cpp_lib_three_way_comparison")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
