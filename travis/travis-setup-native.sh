source travis/travis-find-python.sh

# Travis defines CC and CXX to gcc on Windows, for some reason.
# We need to undo that, or conan will build with the wrong compiler.
export CC=
export CXX=

pushd extern
$CESIUM_PYTHON ./cesium-native/tools/automate.py conan-export-recipes
$CESIUM_PYTHON ./cesium-native/tools/automate.py generate-library-recipes --editable
$CESIUM_PYTHON ./cesium-native/tools/automate.py editable on
conan install . -pr:b=default -pr:h=extern/conan-profiles/$CONAN_PROFILE.jinja
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=cesium-native/build/CesiumUtility/conan/conan_toolchain.cmake
cmake --build build --target install -j 4
popd
