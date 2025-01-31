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
        REF 69b45dae4f64342c5cdc2668b475d29defd79db8
        SHA512 fdea23c5e41b7b01d14dd1a640e5cbe114252396c5f4c42fa1c11ab4d52d77092cf61a1afd45da3d3b431a4d34c2d790576e2c19d19abb3dd6db19447b9d0375
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
vcpkg_cmake_config_fixup(CONFIG_PATH share/cesium-native/cmake PACKAGE_NAME cesium-native)
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include" "${CURRENT_PACKAGES_DIR}/debug/share")

