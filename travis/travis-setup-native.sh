ue4 conan generate
cd extern/cesium-native
python ./tools/automate.py conan-export-recipes
python ./tools/automate.py generate-library-recipes
python ./tools/automate.py conan-export-libraries
cd ../../Source/CesiumNative
conan install . --build=outdated --build=cascade -pr:b=$CONAN_PROFILE -pr:h=$CONAN_PROFILE -s build_type=Release
ue4 conan bake $CONAN_PROFILE
