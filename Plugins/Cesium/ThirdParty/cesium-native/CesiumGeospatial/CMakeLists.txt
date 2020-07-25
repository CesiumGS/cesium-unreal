add_library(CesiumGeospatial "")

configure_cesium_library(CesiumGeospatial)

file(GLOB SOURCES include/CesiumGeospatial/*.h src/*.cpp src/*.h)
file(GLOB PUBLIC_INCLUDES include/CesiumGeospatial/*.h)

target_sources(
    CesiumGeospatial
    PRIVATE
        ${SOURCES}
    PUBLIC
        ${PUBLIC_INCLUDES}
)

target_include_directories(
    CesiumGeospatial
    SYSTEM PUBLIC
        ${PROJECT_SOURCE_DIR}/extern/glm
    PUBLIC
        include
    PRIVATE
        src
)

target_link_libraries(CesiumGeospatial PUBLIC CesiumGeometry CesiumUtility GSL)
