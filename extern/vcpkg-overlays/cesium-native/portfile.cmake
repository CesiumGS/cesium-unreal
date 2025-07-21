vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
  FEATURES
  dependencies-only CESIUM_NATIVE_DEPS_ONLY
)

if(CESIUM_NATIVE_DEPS_ONLY)
  message(STATUS "skipping installation of cesium-native")
  set(VCPKG_POLICY_EMPTY_PACKAGE enabled)
  return()
endif()

vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO CesiumGS/cesium-native
        REF "v${VERSION}"
        SHA512 c5d6146b2c4ddc299eb72c35791f2f798abfa3662b83a94ad978a698811c9a14d79bff6478bdb74582c6e137a717fe4bcf456c8470c16bb0b962094ec77466e9
        HEAD_REF vcpkg-pkg
        )

# Rename sqlite3* symbols to cesium_sqlite3* so they don't conflict with UE's sqlite3,
# which has a bunch of limitations and is not considered public. The sqlite3 port is built this way.

vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
        OPTIONS
                -DCESIUM_USE_EZVCPKG=OFF
                -DPRIVATE_CESIUM_SQLITE=ON
                -DCESIUM_TESTS_ENABLED=OFF
        )

vcpkg_cmake_install()
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
vcpkg_cmake_config_fixup(CONFIG_PATH share/cesium-native/cmake PACKAGE_NAME cesium-native)
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include" "${CURRENT_PACKAGES_DIR}/debug/share")

