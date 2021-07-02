# Releasing a new version of Cesium for Unreal

This is the process we follow when releasing a new version of Cesium for Unreal on GitHub and on the Unreal Engine Marketplace.

## Test the release candidate

* Verify that the cesium-native submodule in the `extern` directory references the expected commit of cesium-native. Update it if necessary. Verify that CI has completed successfully for that commit of cesium-native.
* Wait for CI to complete for the main branch. Verify that it does so successfully.
* Remove any existing copy of the Cesium for Unreal plugin from the engine plugins directory on your system. On Windows this is usually `C:\Program Files\Epic Games\UE_4.26\Engine\Plugins\Marketplace`.
* Download the `plugin-package-combined` for the `main` branch of cesium-unreal. Extract it to your Unreal Engine installation's engine plugins directory. 
* Clone a fresh copy of [cesium-unreal-samples](https://github.com/CesiumGS/cesium-unreal-samples) to a new directory. Launch the Unreal Editor and open `CesiumForUnrealSamples.uproject` from the new cesium-unreal-samples directory.
* Open each level in Content -> CesiumSamples -> Maps and verify it works correctly:
  * Does it open without crashing?
  * Does it look correct?
  * Press Play. Does it work as expected? The billboard in each level should give you a good idea of what to expect.
* Using one of the sample scenes, open the foliage window and create a new foliage type using any engine static mesh. Verify that foliage painting on Cesium World Terrain works correctly.
* Test on other platforms if you can. If you can't (e.g., you don't have a Mac), post a message on Slack asking others to give it at least a quick smoke test.

If all of the above goes well, you're ready to release Cesium for Unreal.

## Update CHANGES.md and tag the cesium-native and cesium-unreal releases

While doing the steps below, make sure no new changes are going into either cesium-unreal or cesium-native that may invalidate the testing you did above. If new changes go in, it's ok, but you should either retest with those changes or make sure they are not included in the release.

* Change the version in `CesiumForUnreal.uplugin`:
  * Increment the `Version` integer property.
  * Change the `VersionName` property to the new three digit, dot-delimited version number. Use [Semantic Versioning](https://semver.org/) to pick the version number.
* Change the `version` property in `package.json` to match the `VersionName` above.
* Verify that cesium-native's CHANGES.md is complete and accurate.
* Verify that cesium-unreal's CHANGES.md is complete and accurate.
* Verify again that cesium-native CI has completed successfully on all platforms.
* Verify again that the submodule reference in cesium-unreal references the correction commit of cesium-native.
* Verify again that cesium-unreal CI has completed successfully on all platforms.
* Tag the cesium-native release, e.g., `git tag -a v0.2.0 -m "0.2.0 release"`
* Push the tag to github: `git push origin v0.2.0`
* Tag the cesium-unreal release, e.g., `git tag -a v1.1.0 -m "1.1.0 release"`
* Push the tag to github: `git push origin v1.1.0`

# Publish the release on GitHub

* Wait for the release tag CI build to complete.
* Download the tag's `plugin-package-combined`. You can find it by switching to the tag in the GitHub UI, clicking the green tick in the header, and then clicking the Details button next to `plugin-package-combined`. While you're here, copy the download URL because you'll need it later.
* Create a new release on GitHub: https://github.com/CesiumGS/cesium-unreal/releases/new. Copy the changelog into it. Follow the format used in previous release. Upload the release ZIP that you downloaded above.

## Publish the Release on Marketplace

1. Open https://publish.unrealengine.com/v2/products. Login with the admin credentials.
2. Navigate to **Products -> Published**.
    ![image](https://user-images.githubusercontent.com/2288659/115271431-58b68180-a10b-11eb-9819-a0bb10c54714.png)
3. Select **Cesium for Unreal**, then scroll all the way to the bottom to the **Product Files** section.
    ![image](https://user-images.githubusercontent.com/2288659/115271629-86032f80-a10b-11eb-9e60-9d838e3a1aec.png)
4. Click the **Submit File Update** button. Confirm the prompts, and you will be directed to this page below.
    * A note here - The **Create New Version** button (don't use this) is for adding a plugin that is for a different version of Unreal Engine. For example if there were different build for 4.25 and 4.26. Even if you accidentally click it, the box for 4.26 will be greyed out. Delete the accidentally added row by clicking the delete icon.
    ![image](https://user-images.githubusercontent.com/2288659/115272156-16417480-a10c-11eb-8b8b-eddbbc3854d6.png)
5. Do the following to update the plugin:
    1. Update the Version Title to match the release version.
    2. Confirm the supported Unreal Engine versions.
    3. Confirm the supported platforms.
    4. Update the version notes with information from [CHANGES.md](https://github.com/CesiumGS/cesium-unreal/blob/main/CHANGES.md).
    5. Copy the CI-generated combined package URL into the **Project File Link** field.
        ![image](https://user-images.githubusercontent.com/2288659/115272024-f0b46b00-a10b-11eb-98ec-c01e40b5e3fb.png)
6. Click **Submit**.
7. This should take you back to the product page, and the **Product Files** section should show **View Pending File Update**. The admin will also recieve an email confirming the submission. The release is now pending Epic's review. If the Marketplace Team reaches out about any issues, those may need resolving, and follow this process again to submit a new zip file for the release.
   ![image](https://user-images.githubusercontent.com/2288659/115330453-140a0500-a162-11eb-95f4-fd7e3f3312b0.png)
8. Once the new release is approved, it does not automatically go live. Take this opportunity to update the product description, images, and other content as needed (product page changes do not require Epic's review).
9. To release it to the marketplace, navigate back to the product page and click **Publish**.
10. Once Epic confirms that the plugin has been updated (likely between hours and days later), return to the Product page and edit the changelog link in the "Technical Details" section to point to the new version.


# Update Cesium for Unreal Samples

Assuming you tested the release candidate as described above, you should have [cesium-unreal-samples](https://github.com/CesiumGS/cesium-unreal-samples) using the updated plugin. You'll use this to push updates to the project.

## Update ion Access Tokens and Project

1. Create a new branch of cesium-unreal-samples. 
2. Delete the Cesium for Unreal Samples token for the release before last, which should expire close to the present date.
3. Create a new access token using the CesiumJS ion account. 
   * The name of the token should match "Cesium for Unreal Samples x.x.x - Delete on September 1st, 2021". The expiry date should be two months later than present. 
   * The scope of the token should be "assets:read" for all assets.
4. Copy the access token you just created. 
5. Open cesium-unreal-samples in Unreal Engine.
6. For each scene in the Maps folder, do the following:
   1. For **all** 3D tilesets in the scene, clear the existing token with the yellow arrow. Add the new access token. Ensure all tilesets are rendering in the scene again.
   2. If the camera has moved at all, press "1" on your keyboard to return the camera to the starting view. Make sure the text box describing the level is clearly framed in the camera view.
   3. Save the scene.
The full list of tilesets that need to be replaced are:
      * Cesium World Terrain in 01_CesiumWorld
      * Cesium World Terrain and Melbourne Photogrammetry in 02_CesiumMelbourne
      * Cesium World Terrain and Aerometrex Denver Photogrammetry in 03_CesiumDenver
      * Cesium World Terrain, Aerometrex Denver, Nearmap Boston, and OSM buildings in 04_MAIN_CesiumSublevels. You will have to load all the sublevels in the Levels menu for all the tliesets to show up in the World Outliner. Make sure to save both 04_MAIN_CesiumSublevels and 04_SublevelParis. 
      * Cesium World Terrain and OSM buildings in 05_CesiumWorld_PostProcessing
7. If the plugin update has replaced any Cesium blueprints that may already exist in one of the scenes, such as DynamicPawn, CesiumSunSky, or similar, replace the old version of the blueprint with the new version, and test the scene with the play button to make sure everything is working. If you're unsure whether the plugin update has resulted in anything that needs to be changed in the Samples, ask the team. 
8. Visit every scene again to make sure that the view is correct and that nothing appears to be missing. 
9. Commit and push your changes, then merge to `main`.

## Publish Cesium for Unreal Samples on Marketplace
Coming soon, presumably the same as above.
