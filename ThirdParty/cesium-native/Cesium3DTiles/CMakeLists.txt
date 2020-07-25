add_library(Cesium3DTiles "")

configure_cesium_library(Cesium3DTiles)

file(GLOB SOURCES include/Cesium3DTiles/*.h src/*.cpp src/*.h)
file(GLOB PUBLIC_INCLUDES include/Cesium3DTiles/*.h)

target_sources(
    Cesium3DTiles
    PRIVATE
        ${SOURCES}
    PUBLIC
        ${PUBLIC_INCLUDES}
)

target_include_directories(
    Cesium3DTiles
    SYSTEM PUBLIC
        ${PROJECT_SOURCE_DIR}/extern/tinygltf
        ${PROJECT_SOURCE_DIR}/extern/glm
        ${PROJECT_SOURCE_DIR}/extern/draco/src
        ${PROJECT_SOURCE_DIR}/extern/uriparser/include
        ${PROJECT_SOURCE_DIR}/extern/build/draco
    PUBLIC
        include
    PRIVATE
        src
)

target_link_libraries(Cesium3DTiles PUBLIC CesiumGeospatial CesiumGeometry CesiumUtility GSL)
