# Releasing a new version of Cesium for Unreal

This is the process we follow when releasing a new version of Cesium for Unreal on GitHub and on the Unreal Engine Marketplace.

## Verify the code

* Update any hard coded API keys in the code, including:
  * `testIonToken` in `CesiumSceneGeneration.cpp`. Make sure it matches the key in the samples project.
* Verify that the cesium-native submodule in the `extern` directory references the expected commit of cesium-native. Update it if necessary. Verify that CI has completed successfully for that commit of cesium-native.
* Wait for CI to complete for `main`. Verify that it does so successfully.

## Test the release candidate

For the following instructions, replace `v2.0.0` with the actual version number you are targeting. Let `X` represent the minor version of Unreal Engine 5 you're currently testing.

* Remove all existing copies of the Cesium for Unreal plugin from the engine plugin directories on your system. On Windows this is usually `C:\Program Files\Epic Games\UE_5.X\Engine\Plugins\Marketplace`.
* In the `main` branch, go to https://github.com/CesiumGS/cesium-unreal/actions and click the most recent build of the branch (it should be near the top). Scroll down to **Artifacts** and download the artifact that doesn't have an operating system in its name, e.g., `CesiumForUnreal-5X-v2.0.0`. It will also be the largest artifact. Extract it to `C:\Program Files\Epic Games\UE_5.X\Engine\Plugins\Marketplace`.
* Clone a fresh copy of [cesium-unreal-samples](https://github.com/CesiumGS/cesium-unreal-samples) to a new directory. To test in a different version, right-click on the `CesiumForUnrealSamples.uproject` file and select "Switch Unreal Engine Version". Double-click that same file to open it in the Editor.
* Open each level in Content -> CesiumSamples -> Maps and verify it works correctly.
  * Does it open without crashing?
  * Does it look correct?
  * Press Play. Does each sample work as expected? The billboard in each level should give you a good idea of what to expect.
  * For `04_MAIN_CesiumSublevels`, make sure that the sub-levels are loading as expected. Ensure that no other tilesets or objects showing outside of their intended sub-levels.
* Using one of the sample scenes, open the foliage window and create a new foliage type using any engine static mesh. Verify that foliage painting on Cesium World Terrain works correctly.
* Test on other platforms and other versions of Unreal Engine if you can. If you can't (e.g., you don't have a Mac), post a message on Slack asking others to give it at least a quick smoke test.

Make sure to do this for all three currently-supported versions of Unreal Engine. If all of the above goes well, you're ready to release Cesium for Unreal.

## Update CHANGES.md and tag the cesium-native and cesium-unreal releases

While doing the steps below, make sure no new changes are going into either cesium-unreal or cesium-native that may invalidate the testing you did above. If new changes go in, it's ok, but you should either retest with those changes or make sure they are not included in the release.

* Change the version in `CesiumForUnreal.uplugin`:
  * Increment the `Version` integer property.
  * Change the `VersionName` property to the new three digit, dot-delimited version number. Use [Semantic Versioning](https://semver.org/) to pick the version number.
* Change the `version` property in `package.json` to match the `VersionName` above.
* Verify that cesium-native's CHANGES.md is complete and accurate.
* Verify that cesium-unreal's CHANGES.md is complete and accurate.
* Verify again that cesium-native CI has completed successfully on all platforms.
* Verify again that the submodule reference in cesium-unreal references the correct commit of cesium-native.
* Verify again that cesium-unreal CI has completed successfully on all platforms.
* Tag the cesium-native release, e.g., `git tag -a v0.2.0 -m "0.2.0 release"`, replacing the version number with your actual version.
* Push the tag to GitHub: `git push origin v0.2.0`
* Tag the cesium-unreal release, e.g., `git tag -a v2.0.0 -m "2.0.0 release"`, replacing the version number with your actual version.
* Push the tag to GitHub: `git push origin v2.0.0`

# Publish the release on GitHub

* Wait for the release tag CI build to complete.
* Download the built plugin packages for the tags, as you did above for the main branches.
* Create a new release on GitHub: https://github.com/CesiumGS/cesium-unreal/releases/new. Copy the changelog into it. Follow the format used in previous release and upload the release .zips that you downloaded above. Make sure you upload the .zips for all three currently-supported versions.

## Publish the Release on Marketplace

1. Open https://publish.unrealengine.com/. Login with the unreal@cesium.com credentials.
2. Navigate to **Products -> Published**.
    ![image](https://user-images.githubusercontent.com/2288659/115271431-58b68180-a10b-11eb-9819-a0bb10c54714.png)
3. Select **Cesium for Unreal**, then scroll all the way to the bottom to the **Product Files** section.
    ![image](https://user-images.githubusercontent.com/2288659/115271629-86032f80-a10b-11eb-9e60-9d838e3a1aec.png)
4. Click the **Submit File Update** button. Confirm the prompts, and you will be directed to this page below.
    * A note here - The **Create New Version** button (don't use this) is for adding a plugin that is for a different version of Unreal Engine. For example, if there were different builds for 4.25 and 4.26. Even if you accidentally click it, the box for 4.26 will be greyed out. Delete the accidentally added row by clicking the delete icon.
    ![image](https://user-images.githubusercontent.com/2288659/115272156-16417480-a10c-11eb-8b8b-eddbbc3854d6.png)
5. Do the following to update the plugin:
    1. Update the Version Title to match the release version.
    2. Confirm the supported Unreal Engine versions.
    3. Confirm the supported platforms.
    4. Update the version notes to reference the appropriate version of [CHANGES.md](https://github.com/CesiumGS/cesium-unreal/blob/main/CHANGES.md).
    5. Copy download URLs from the GitHub release pages you created above into the **Project File Link** field.
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
2. Create a new access token using the CesiumJS ion account.
   * The name of the token should match the format "Cesium for Unreal Samples x.x.x - Delete on September 1st, 2021". The expiry date should be set to two months after the date it was created.
   * The scope of the token should be "assets:read" for all assets.
3. Copy the access token you just created.
4. Open cesium-unreal-samples in Unreal Engine.
5. For the `ion.cesium.com` server, use the **Token** window to paste the new token into the field for **Specify a token**.
6. If the plugin update has replaced any Cesium blueprints that may already exist in one of the scenes, e.g., DynamicPawn or CesiumSunSky, replace the old version of the blueprint with the new version, and test the level with the play button to make sure everything is working. If you're unsure whether the plugin update has resulted in anything that needs to be changed in the Samples, ask the team.
7. Visit every level again to make sure that the view is correct and that nothing appears to be missing. Repeat the same tests that you did while testing the Cesium for Unreal release.
8. Commit and push your changes. Create a PR to merge to `main` and tag a reviewer.

## Publish the Cesium for Unreal Samples release on GitHub

After the update has been merged to `main`, do the following:

1. Pull and check out the latest version of `main` from GitHub, and then tag the new release by doing the following:
  * `git tag -a v1.10.0 -m "v1.10.0 release"`
  * `git push origin v1.10.0`
2. Wait for the continuation integration build to complete for the tag.
3. Switch to the tag in the GitHub UI by visiting the repo, https://github.com/CesiumGS/cesium-unreal-samples, clicking the combo box where it says "main", switching to the Tags tab, and selecting the new tag that you created above.
4. Click the green tick ✔️ at the top of the list of files and click the "Details" link next to "project-package". This will download the built package to your computer. Also copy this URL (by right-clicking the Details link and choosing `Copy link address`) because you will need it later.
5. Create a new release on GitHub: https://github.com/CesiumGS/cesium-unreal-samples/releases/new. Select the tag you created above. Add a changelog in the body to describe recent updates. Follow the format used in previous release.
6. Upload the release ZIP that you downloaded above.
7. Publish the release.

## Publish Cesium for Unreal Samples on Marketplace

**DO NOT do this step until Epic has accepted the updated plugin!** Otherwise, Epic may release the Samples update on the Marketplace before the plugin itself is updated. The updated Samples project is not guaranteed to work with old versions of the plugin, so this  can leave users in a broken state. On the other hand, the plugin is usually backwards compatible, so the old Samples version will work fine with the new plugin version. Therefore, always make sure the plugin is published to the Marketplace first.

1. Open https://publish.unrealengine.com/. Login with the admin credentials.
2. Navigate to **Products -> Published**.
3. Select **Cesium for Unreal Samples**, then scroll all the way to the bottom to the **Product Files** section.
4. Click the **Submit File Update** button. Confirm the prompts, and you will be directed to this page below.
    * A note here - The **Create New Version** button (don't use this) is for adding a plugin that is for a different version of Unreal Engine. For example if there were different build for 4.25 and 4.26. Even if you accidentally click it, the box for 4.26 will be greyed out. Delete the accidentally added row by clicking the delete icon.
5. Do the following to update the plugin:
    1. Update the Version Title to match the release version.
    2. Replace the Project File Link with the download link you copied from the github continuation integration green tick.
    3. Confirm the supported Unreal Engine versions.
    4. Confirm the supported platforms.
    5. Replace the Version Notes with the link to the published release.(Likely `https://github.com/CesiumGS/cesium-unreal-samples/releases/tag/vX.X.X`)
  ![image](https://user-images.githubusercontent.com/39537389/126185273-f4df4437-a9c9-477b-85a3-10e75699c26c.png)
6. Click **Submit**.
7. This should take you back to the product page, and the **Product Files** section should show **View Pending File Update**. The admin will also recieve an email confirming the submission. The release is now pending Epic's review. If the Marketplace Team reaches out about any issues, those may need resolving, and follow this process again to submit a new zip file for the release.
8. Once the new release is approved, it does not automatically go live. Take this opportunity to update the product description, images, and other content as needed (product page changes do not require Epic's review).
9. To release it to the marketplace, navigate back to the product page and click **Publish**.
10. After the new release is accepted by Epic, delete the Cesium for Unreal Samples token in Cesium ion for the release before last, which should expire close to the present date.
