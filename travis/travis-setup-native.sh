ue4 conan generate
cd extern/cesium-native
python ./tools/automate.py conan-export-recipes
python ./tools/automate.py generate-library-recipes
python ./tools/automate.py conan-export-libraries
cd ../../Source/CesiumNative
ue4 conan bake ue
