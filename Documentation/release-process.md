# Releasing a new version of Cesium for Unreal

This is the process we follow when releasing a new version of Cesium for Unreal on GitHub and on the Unreal Engine Marketplace.

* Verify that cesium-native's CHANGES.md is complete and accurate.
* Verify that cesium-native CI has completed successfully on all platforms.
* Verify that the submodule reference in cesium-unreal references the correction commit of cesium-native.
* Verify that cesium-unreal builds against the nominated cesium-native version and that everything works as expected.
* Tag the cesium-native release, e.g., `git tag -a v0.2.0 -m "0.2.0 release"`
* Push the tag to github: `git push origin v0.2.0`
* Verify that cesium-unreal's CHANGES.md is complete and accurate.
* Tag the cesium-unreal release, e.g., `git tag -a v1.1.0 -m "1.1.0 release"`
* Push the tag to github: `git push origin v1.1.0`
* Create a new release on GitHub: https://github.com/CesiumGS/cesium-unreal/releases/new
* TODO: add instructions for updating on the Marketplace.
