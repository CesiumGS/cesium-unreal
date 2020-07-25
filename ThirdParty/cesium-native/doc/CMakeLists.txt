find_package(Doxygen)

if (DOXYGEN_FOUND)
    doxygen_add_docs(
        cesium-native-docs
        ../Cesium3DTiles/include
        ../CesiumGeospatial/include
        ../CesiumGeometry/include
        ../CesiumUtility/include
    )
endif()
