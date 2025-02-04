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
        REF d5e261baf93ec28573508495b55020e222689320
        SHA512 740352db674d1663421d239c369f2a6fb2f4444529be609afa7636be5f3ebb0534d3922e1319e0c3ab481ce9431171e1dc06650dd51b0fe9e06e3dbb852f4726
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

