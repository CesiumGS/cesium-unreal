# Releasing a new version of Cesium for Unreal {#release-process}

This is the process we follow when releasing a new version of Cesium for Unreal on GitHub and on the Unreal Engine Marketplace.

## Prepare Cesium Native for Release

[cesium-native](https://github.com/CesiumGS/cesium-native) is used as a git submodule of cesium-unreal. So the first step in releasing Cesium for Unreal is to make sure that Cesium Native is ready for release. See the [Cesium Native release guide](#native-release-process) for instructions. You may want to wait to create the new Cesium Native tag until after you have tested the Cesium for Unreal release.

## Prepare Cesium for Unreal for Release

1. Update the cesium-native submodule reference to point to the latest commit that you pushed above.
   - Enter the `extern/cesium-native` directory and pull the latest changes and checkout the main branch as normal.
   - To update the submodule reference, from the cesium-unreal root, run: `git add extern/cesium-native`
2. Verify that `CHANGES.md` is complete and accurate.
   - Give the header of the section containing the latest changes an appropriate version number and date.
   - Diff main against the previous released version. This helps catch changes that are missing from the changelog, as well as changelog entries that were accidentally added to the wrong section.
   - Use [Semantic Versioning](https://semver.org/) to pick the version number.
   - Don't forget to add a note to the end of the version section indicating the cesium-native version number change. Use a previous version as the template for this and update the "from" and "to" version numbers accordingly.
3. Change the version in `CesiumForUnreal.uplugin`:
   - Increment the `Version` integer property.
   - Change the `VersionName` property to the new three digit, dot-delimited version number.
4. Create a new Cesium ion token for this release under the "CesiumJS" account:
   - Visit https://ion.cesium.com.
   - Log in using your own `@cesium.com` email address.
   - Click your name in the top-right corner and choose "Switch to CesiumJS". If you don't see this option, ask someone on the ion team to add you to the CesiumJS account.
   - Click "Access Tokens".
   - Create a new token named "Cesium for Unreal Samples vX.Y.Z - Delete on MONTH DAY, YEAR", using real values for the version number and date. The date should be the date of the release two months out, using the month's name rather than its number.
   - Enable _only_ the `assets:read` and `geocode` scopes on the new token.
   - Set the other properties as follows (these should be the defaults):
     - `Allow URLs`: "All Urls"
     - `Resources`: "All assets"
   - Copy the new token string into the `testIonToken` variable in `CesiumSceneGeneration.cpp`.
5. Commit these changes. You can push them directly to `main`.
6. Before continuing, verify that CI passes for all platforms and Unreal Engine versions.

## Optimistically upload the release packages to the GitHub releases page

Cesium for Unreal has several release packages, and they're fairly large, so it can take a while to upload them all to the GitHub Releases page. For that reason, it is helpful to start the process now, even though we haven't tested the release yet.

1. Go to https://github.com/CesiumGS/cesium-unreal/actions.
2. Click the most recent build of the `main` branch (it should be near the top). This should be the build for the commit you pushed in the previous section. If it's still running, wait. If it failed, you'll need to fix it.
3. Scroll down to **Artifacts**.
4. Download the artifacts that don't have an operating system in its name, e.g., `CesiumForUnreal-5X-main` and `CesiumForUnreal-5X-SourceOnly-main`. There will typically be 6 of these in total: a full build and a "SourceOnly" build for each of the three supported Unreal Engine versions.
5. Rename each of the files downloaded above, replacing `main` in each with the actual release version number. For example, `CesiumForUnreal-57-SourceOnly-main.zip` might become `CesiumForUnreal-57-SourceOnly-v2.22.0.zip`.
6. Create a new release by visiting https://github.com/CesiumGS/cesium-unreal/releases/new.
7. Leave the release tag blank for now, as we haven't created it yet.
8. Set `Release Title` to "Cesium for Unreal v2.22.0", updating the version number as appropriate.
9. Copy the "release notes" section from a previous release, which you can find by visiting https://github.com/CesiumGS/cesium-unreal/releases/latest and clicking the Edit button. Be careful not to change or save the previous release!
10. Update the version numbers as appropriate in the top section. Replace the changelog section with the actual changelog entries from this release. Copy it from `CHANGES.md`.
11. Upload all 6 release package files by dragging them into the "Attach binaries" box. This will take a little while and doesn't have great feedback.
12. Click Save Draft. Be careful not to publish it yet.
13. Visit the [releases page](https://github.com/CesiumGS/cesium-unreal/releases) and you'll see the new draft release at the top. Expand the "Assets" section you should see the files you uploaded above.
14. Take a moment to verify that the SHA256 listed for each file is identical to the SHA256 for the same file on the Artifacts page above. If they are, this proves the release package hasn't been inadvertently modified while you had a copy of it on your local machine. If they're different, stop the release process immediately!

If the uploaded packages are later found to have problems during testing, this step will need to be repeated.

## Install the Updated Packages

1. Remove all existing copies of the Cesium for Unreal plugin from the engine plugin directories on your system, one for each version of Unreal Engine. On Windows this is usually `C:\Program Files\Epic Games\UE_5.X\Engine\Plugins\Marketplace`.
2. Extract the downloaded full ZIP files (_not_ the `SourceOnly` ones) to the corresponding Unreal Engine Marketplace directory. For example, `CesiumForUnreal-57-v2.22.0.zip` should be extracted to `C:\Program Files\Epic Games\UE_5.7\Engine\Plugins\Marketplace` on Windows.

## Update the "Cesium for Unreal Samples" Project

1. Clone a fresh copy of [cesium-unreal-samples](https://github.com/CesiumGS/cesium-unreal-samples) to a new directory.
2. Open the Samples project in the oldest supported version of Unreal Engine. It is essential that you don't use a newer version for this step.
3. On the Cesium panel, click the Tokens button.
4. In the "Specify a token" section, replace the token string with the one you created on Cesium ion above. This is the same one that you pasted into `CesiumSceneGeneration.cpp`.
5. Click "Use as Project Default Token". Ensure that the tilesets reload successfully with the new token.
6. In rare cases, the updated plugin may modify existing Blueprints in the Samples project based on its `CoreRedirects`. In this case, you should open each sample level in turn to allow the redirects to be applied, and then save them.
6. Exit Unreal Engine. Save the changes when prompted.
7. Commit `Content/CesiumSettings/CesiumIonServers/CesiumIonSaaS.uasset`. Also commit other files that were changed via `CoreRedirects`. Do not commit other extraneous changes. You can push the changes directly to `main`.

## Test the release candidate

Cesium for Unreal supports a large number of platforms and Unreal Engine versions, so it is a huge amount of work to manually test them all. As a compromise, we generally:

- Thoroughly test Editor and Player builds on Windows on the oldest and newest versions of Unreal Engine that we currently support.
- Thoroughly test other platforms when a release has an above-average chance of breaking on those other platforms. For example, if we added platform-specific code or added new shaders that could have compatibility problems on mobile platforms.
- Spot check one or two other platforms / versions each release, and try to switch up which ones those are.

To test the release, do the following:

1. Clone a fresh copy of [cesium-unreal-samples](https://github.com/CesiumGS/cesium-unreal-samples) to a new directory.
2. The Samples project will be configured to use the oldest supported version of Unreal Engine. To test in a different version, right-click on the `CesiumForUnrealSamples.uproject` file and select "Switch Unreal Engine Version". On Windows 11, you may need to choose "Show More Options" first.
3. Double-click the `CesiumForUnrealSamples.uproject` file to open it in the Editor.
4. Verify that the version number in the lower-right corner of the Cesium panel is what you expect.
5. Open each level in Content -> CesiumSamples -> Maps and verify it works correctly.
   - Does it open without crashing?
   - Does it look correct?
   - Press Play. Does each sample work as expected? The billboard in each level should give you a good idea of what to expect.
   - For `04_MAIN_CesiumSublevels`, make sure that the sub-levels are loading as expected. Ensure that no other tilesets or objects are showing outside of their intended sub-levels.
6. Using one of the sample scenes, open the foliage window and create a new foliage type using any engine static mesh. Verify that foliage painting on Cesium World Terrain works correctly.
7. Verify that signing in to Cesium ion works correctly.
8. Test packaging, especially in the Shipping configuration.
   - Select the shipping configuration by clicking Platforms -> Windows -> Shipping.
   - Package the project by clicking Platforms -> Windows -> Package Project. Create a new directory for the output.
   - Verify that the packaged project runs successfully.

## Release!

1. Tag the cesium-native release if you haven't already, and push the tag. See the [Cesium Native release guide](#native-release-process) for instructions.
2. Tag the cesium-unreal release, and push the tag. Be sure you're tagging the exact commit that you tested.
   - `git tag -a v2.22.0 -m "v2.22.0 release"`
   - `git push origin v2.22.0`
3. Publish the release on GitHub.
   - Visit https://github.com/CesiumGS/cesium-unreal/releases.
   - Find the Draft release you previously created and click its Edit button.
   - Select the tag you just created and pushed.
   - Double-check that the other details look good.
   - Click "Publish release".
4. Publish the reference documentation. A CI job automatically publishes the documentation to the web site at https://cesium.com/learn/cesium-unreal/ref-doc/ when it is merged into the [cesium.com](https://github.com/CesiumGS/cesium-unreal/tree/cesium.com) branch. So do the following:
   - `git checkout cesium.com`
   - `git pull --ff-only`
   - `git merge v2.22.0 --ff-only`
   - `git push`
   - `git checkout main`
4. Publish the release on Epic's Fab marketplace.
   - Visit https://fab.com.
   - Sign in as `unreal@cesium.com`.
   - Go to Publish -> Listings -> Cesium for Unreal.
   - Click "Unreal Engine" under the "Included Formats" section in the left sidebar.
   - Scroll to the end of the "Project Versions" section at the top of the page, and find the three most recent Unreal Engine versions.
   - Edit each of these versions as follows:
     - Change the "Version Title" to match the new version number.
     - Change the "Project file link" to the URL of the "SourceOnly" package for that particular version in the GitHub Release you published above.
     - Change the "Version notes" field with the new version number and link to the changelog for that version.
   - Farther down the page, in the "Technical Details" section, update the version number, release date, and link to the changelog.
   - Double-check that everything looks good (in particular, that the links to the SourceOnly packages work) and then click "Submit for Review".
   - Select the option to publish immediately after the release is approved.
5. Wait for Epic to publish the new release.
   - This usually takes somewhere between a couple of hours and a couple of days.
   - Unfortunately, Epic no longer notifies us when this is done. To check, go to Publish -> Listings on fab.com and look for a "pending update" badge over Cesium for Unreal. Once this disappears, the release has been published.

## Final Steps

Once Epic has published the new release, there are a few more steps to complete.

**DO NOT do these steps until Epic has accepted the updated plugin!** Otherwise, Epic may release the Samples update on the Marketplace before the plugin itself is updated. The updated Samples project is not guaranteed to work with old versions of the plugin, so this can leave users in a broken state. On the other hand, the plugin is usually backwards compatible, so the old Samples version will work fine with the new plugin version. Therefore, always make sure the plugin is published to the Marketplace first.

1. Verify Epic's build.
   - Delete the manually-installed build that you tested previously. These are found in `C:\Program Files\Epic Games\UE_5.X\Engine\Plugins\Marketplace`.
   - Restart the Epic Launcher.
   - Uninstall Cesium for Unreal from any engine version in which it is already installed.
   - Install Cesium for Unreal into every engine version.
   - Repeat an abbreviated version of the testing you did previously against this new Epic-built version. First and foremost, verify that the version shown on the Cesium panel is correct.
2. Publish Cesium for Unreal Samples to GitHub releases.
   - Create and push a git tag for the Sample repo. Be sure you're tagging the exact commit that you tested.
     - `git tag -a v2.22.0 -m "v2.22.0 release"`
     - `git push origin v2.22.0`
   - Wait for CI to run on the pushed tag.
   - Go to https://github.com/CesiumGS/cesium-unreal/actions.
   - Click the build of the tag you just pushed.
   - Scroll down to **Artifacts** and download the artifact (there should be only one).
   - The downloaded file's name will end with `.zip.zip`. Rename it to be just `.zip`.
   - Create a new release by visiting https://github.com/CesiumGS/cesium-unreal-samples/releases/new.
   - Select the tag you created above.
   - Enter "Cesium for Unreal Samples v2.22.0" for the title, updating the version number as appropriate.
   - Copy the release notes from a previous release, and modify them as appropriate.
   - Drag the download release ZIP file into the "Attach binaries" box and wait for it to upload.
   - Click "Publish Release".
3. Publish Cesium for Unreal Samples to Fab.
   - Visit https://fab.com and sign in as `unreal@cesium.com` as before.
   - Go to Publish -> Listings -> Cesium for Unreal Samples.
   - Click "Unreal Engine" under the "Included Formats" section in the left sidebar.
   - For the Samples project, there should be only one "Project Version".
     - Update the "Version Title" and "Version Notes" for the new version.
     - Change the "Project file link" to the URL of the release ZIP you published on the GitHub Releases page.
   - Click "Submit for review".
4. Delete the old Cesium ion token.
   - On the https://ion.cesium.com "Access Tokens" page, find the token that says something like: "Cesium for Unreal Samples vX.Y.Z - Delete on [today's date]".
   - Look at the token's recent usage. It won't be zero, but it should be significantly lower than the newer token.
   - Delete the old token.
   - The goal is to ensure that a copy of the Cesium for Unreal Samples project downloaded at any time will continue to work for a month. That's why we delete the token from two releases ago. However, if for whatever reason we skip releasing for a month, we should _not_ delete the old token on the date indicated in the description, and instead delete it a month later.
