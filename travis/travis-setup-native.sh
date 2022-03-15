source travis/travis-find-python.sh

$CESIUM_PIP install conan


pushd extern
$CESIUM_PYTHON ./cesium-native/tools/automate.py conan-export-recipes
$CESIUM_PYTHON ./cesium-native/tools/automate.py generate-library-recipes --editable
conan editable add conan/recipes/openssl-ue4.26 openssl/UE4.26
conan editable add conan/recipes/openssl-ue4.27 openssl/UE4.27
$CESIUM_PYTHON ./cesium-native/tools/automate.py editable on
# Force build of uriparser because the Conan Center version uses reallocarray, which does not exist in UE's CentOS 7
conan install . -pr:b=default -pr:h=./conan-profiles/$CONAN_PROFILE.jinja --build=uriparser --build=outdated --build=cascade
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=cesium-native/build/CesiumUtility/conan/conan_toolchain.cmake $EXTRA_CONFIGURE_ARGS
cmake --build build --target install -j 4 --config Release
popd
