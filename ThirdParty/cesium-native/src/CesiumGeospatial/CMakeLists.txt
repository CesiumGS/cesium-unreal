add_library(CesiumGeospatial "")

configure_cesium_library(CesiumGeospatial)

file(GLOB SOURCES *.cpp *.h)
file(GLOB PUBLIC_INCLUDES ${PROJECT_SOURCE_DIR}/include/CesiumGeospatial/*.h)

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
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        private
)

target_link_libraries(CesiumGeospatial CesiumGeometry CesiumUtility GSL)
