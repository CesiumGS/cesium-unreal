add_library(CesiumGeometry "")

configure_cesium_library(CesiumGeometry)

file(GLOB SOURCES include/CesiumGeometry/*.h src/*.cpp src/*.h)
file(GLOB PUBLIC_INCLUDES include/CesiumGeometry/*.h)

target_sources(
    CesiumGeometry
    PRIVATE
        ${SOURCES}
    PUBLIC
        ${PUBLIC_INCLUDES}
)

target_include_directories(
    CesiumGeometry
    SYSTEM PUBLIC
        ${PROJECT_SOURCE_DIR}/extern/glm
    PUBLIC
        include
    PRIVATE
        src
)

target_link_libraries(CesiumGeometry PUBLIC CesiumUtility GSL)
