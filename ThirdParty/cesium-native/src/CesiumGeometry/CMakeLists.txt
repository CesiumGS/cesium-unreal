add_library(CesiumGeometry "")

configure_cesium_library(CesiumGeometry)

file(GLOB SOURCES *.cpp *.h)
file(GLOB PUBLIC_INCLUDES ${PROJECT_SOURCE_DIR}/include/CesiumGeometry/*.h)

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
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        private
)

target_link_libraries(CesiumGeometry PUBLIC CesiumUtility GSL)
