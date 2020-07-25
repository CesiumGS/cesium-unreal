add_library(CesiumUtility "")

configure_cesium_library(CesiumUtility)

file(GLOB SOURCES include/CesiumUtility*.h src/*.cpp src/*.h)
file(GLOB PUBLIC_INCLUDES include/CesiumUtility/*.h)

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
        include
    PRIVATE
        src
)
