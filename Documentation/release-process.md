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
* Create a new release on GitHub: https://github.com/CesiumGS/cesium-unreal/releases/new. Upload the release ZIP for each platform from CI.

## Updating the Release on Marketplace

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
    5. We need to combined the Windows and MacOS builds into a single file. Download the built packages from GitHub, combine the directories, and create a single zip file named `CesiumForUnreal-vX.Y.Z.zip`. Upload this to AWS S3 bucket for Cesium for Unreal builds. Copy this public link into the **Project File Link** field.
    ![image](https://user-images.githubusercontent.com/2288659/115272024-f0b46b00-a10b-11eb-98ec-c01e40b5e3fb.png)
6. Click **Submit**.
7. This should take you back to the product page, and the **Product Files** section should show **View Pending File Update**. The admin will also recieve an email confirming the submission. The release is now pending Epic's review. If the Marketplace Team reaches out about any issues, those may need resolving, and follow this process again to submit a new zip file for the release.
   ![image](https://user-images.githubusercontent.com/2288659/115330453-140a0500-a162-11eb-95f4-fd7e3f3312b0.png)
8. Once the new release is approved, it does not automatically go live. Take this opportunity to update the product description, images, and other content as needed (product page changes do not require Epic's review).
9. To release it to the marketplace, navigate back to the product page and click **Publish**.
