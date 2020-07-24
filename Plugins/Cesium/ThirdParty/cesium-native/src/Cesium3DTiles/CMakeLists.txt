add_library(Cesium3DTiles "")

configure_cesium_library(Cesium3DTiles)

file(GLOB SOURCES *.cpp *.h)
file(GLOB PUBLIC_INCLUDES ${PROJECT_SOURCE_DIR}/include/Cesium3DTiles/*.h)

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
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        private
)

target_link_libraries(Cesium3DTiles CesiumGeometry CesiumUtility GSL)
