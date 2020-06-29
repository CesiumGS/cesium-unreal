file(GLOB SOURCES *.cpp *.h)

target_sources(
    cesium-native-tests
    PRIVATE
        ${SOURCES}
)
