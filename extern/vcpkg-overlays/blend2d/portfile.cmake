vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO blend2d/blend2d
  REF d2027ebfd6aaf53b190b6b3b497425fc85f14251 # commited on 2025-03-08
  SHA512 f7ecda8280290a1692bbec618522eccf1d74f79c688affc687848459c06762e405ad2f319845a548d478723ed8bf8db609e4691bc335f364baceb20d9d3aa597
  HEAD_REF master
)

string(COMPARE EQUAL "${VCPKG_LIBRARY_LINKAGE}" "static" BLEND2D_STATIC)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
  INVERTED_FEATURES
    jit        BLEND2D_NO_JIT
)

# iOS will not allow us to use JIT, so we need to disable it in Blend2D.
# Once we're using manifest mode for Unreal, we can get rid of this overlay port and use the above feature.
if(IOS)
    set(IOS_DISABLE_JIT "-DBLEND2D_NO_JIT")
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        "-DBLEND2D_STATIC=${BLEND2D_STATIC}"
        "-DBLEND2D_NO_FUTEX=OFF"
        "-DBLEND2D_EXTERNAL_ASMJIT=ON"
        ${IOS_DISABLE_JIT}
        ${FEATURE_OPTIONS}
)

vcpkg_cmake_install()
vcpkg_copy_pdbs()

vcpkg_cmake_config_fixup(CONFIG_PATH "lib/cmake/${PORT}")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

if(VCPKG_LIBRARY_LINKAGE STREQUAL "static")
    vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/include/blend2d/api.h"
        "#if !defined(BL_STATIC)"
        "#if 0"
    )
    vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/include/blend2d-debug.h"
        "#if defined(BL_STATIC)"
        "#if 1"
    )
endif()

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.md")
