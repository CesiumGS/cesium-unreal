add_library(CesiumUtility "")

configure_cesium_library(CesiumUtility)

file(GLOB SOURCES *.cpp *.h)
file(GLOB PUBLIC_INCLUDES ${PROJECT_SOURCE_DIR}/include/CesiumUtility/*.h)

target_sources(
    CesiumUtility
    PRIVATE
        ${SOURCES}
    PUBLIC
        ${PUBLIC_INCLUDES}
)

target_include_directories(
    CesiumUtility
    SYSTEM PUBLIC
        ${PROJECT_SOURCE_DIR}/extern/glm
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
