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
        REF 70b8d69413b5fd8e5d8758ede46cc3be5e4f4f19
        SHA512 46c94437c35964cd878fbae46f5b1890b33779b4afa9c30d9fbfe8f10f598bc68ed63ee820fb3f1eb7438499962e45db4a64b92b72c050f8c8e0285e66614b2e
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

