vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO zlib-ng/zlib-ng
    REF "${VERSION}"
    SHA512 59ef586c09b9a63788475abfd6dd59ed602316b38f543f801bea802ff8bec8b55a89bee90375b8bbffa3bdebc7d92a00903f4b7c94cdc1a53a36e2e1fd71d13a
    HEAD_REF develop
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        "-DZLIB_FULL_VERSION=${ZLIB_FULL_VERSION}"
        -DZLIB_ENABLE_TESTS=OFF
        -DWITH_NEW_STRATEGIES=ON
        # Disable ARMv6 instructions. We don't need this because we only run on 64-bit ARM (v8),
        # which has better instructions. zlib-ng has a bug that makes it try to use these v6
        # instructions even though they're not available. An attempt to fix it was made in this
        # PR: https://github.com/zlib-ng/zlib-ng/pull/1617
        # But it doesn't work in our Android builds because the dependent option
        # "NOT ARCH STREQUAL \"aarch64\"" that is meant to set `WITH_ARMV6` to FALSE is not
        # triggered because our ARCH is `aarch64-none-linux-android21`. It's not clear if this
        # is something quirky about our environment or if the fix is just not robust.
        # Either way, forcing WITH_ARMV6=OFF here fixes the probelm and should be reasonable
        # on all platforms that Cesium for Unreal supports.
        -DWITH_ARMV6=OFF
    OPTIONS_RELEASE
        -DWITH_OPTIM=ON
)
vcpkg_cmake_install()
vcpkg_copy_pdbs()
vcpkg_fixup_pkgconfig()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share"
                    "${CURRENT_PACKAGES_DIR}/debug/include"
)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.md")
