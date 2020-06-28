file(GLOB SOURCES *.cpp *.h)
file(GLOB PUBLIC_INCLUDES ${PROJECT_SOURCE_DIR}/include/Cesium3DTiles/*.h)

target_sources(
    cesium-native
    PRIVATE
        ${SOURCES}
    PUBLIC
        ${PUBLIC_INCLUDES}
)

target_include_directories(
    cesium-native
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(cesium-native
    PUBLIC
        CESIUM3DTILES_API=
)
