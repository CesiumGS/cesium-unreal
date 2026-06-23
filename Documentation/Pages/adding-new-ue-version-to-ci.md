# Adding a new Unreal Engine version to Github Actions continuous integration {#adding-unreal-version-ci}

Setting up a new version of Unreal Engine on CI requires the following at a high level:

1. Creating a ZIP for the new UE version for each platform (Windows, macOS, and Linux)
2. Uploading the ZIP files to S3.
3. Adding new CI jobs for the new version.

## Creating Unreal Engine ZIP files

This section explains how to create an Unreal Engine ZIP file for CI on each platform.

### Windows

Using Epic Launcher, install the new version of Unreal Engine. 

> **NOTE:** If you've already installed it, you should uninstall it and delete its installation directory before proceeding.

On the Choose Install Location panel, click Options. On that panel:

1. Deselect "Template and Feature Packs"
2. Select "Android" under "Target Platforms"

Wait for the installation to complete. Then, _before launching it_:

1. Find the installation directory. It will be something like `C:\Program Files\Epic Games\UE_5.8`.
2. Use 7-Zip to create a ZIP file from the UE directory (e.g., `UE_5.8`):
   - You should ZIP the entire directory, _not_ its contents. That is, the root of the ZIP file should contain only a `UE_5.8` directory or similar.
   - Set the "Compression level" to `9 - Ultra`
   - For the ZIP filename, use something like `UE_5.8.zip`.

After creating the ZIP file, you may want to go back and install the "Template and Feature Packs" option that was deselected above. This isn't needed on CI, and it's useful to save the bytes, but you probably want it locally.

### macOS

Using Epic Launcher, install the new version of Unreal Engine. 

> **NOTE:** If you've already installed it, you should uninstall it and delete its installation directory before proceeding.

On the Choose Install Location panel, click Options. On that panel:

1. Deselect "Template and Feature Packs"
2. Select "IOS" under "Target Platforms"

Wait for the installation to complete. Then, _before launching it_:

1. Find the installation directory. It is usually found in something like `/Users/Shared/Epic Games/UE_5.8`.
2. Create the ZIP file by running the following (adjusting the path if necessary):
   - `cd /Users/Shared/Epic\ Games`
   - `zip -9 -r UE_58-mac.zip UE_5.8`

After creating the ZIP file, you may want to go back and install the "Template and Feature Packs" option that was deselected above. This isn't needed on CI, and it's useful to save the bytes, but you probably want it locally.

### Linux

Epic provides ZIP files for Linux, so we don't need to create them ourselves. Download one from:
https://www.unrealengine.com/linux

## Upload the ZIP files to S3

You will need the aws command-line utility configured to be able to access the `s3://cesium-unreal-engine` bucket. The process for doing this is the same as for publishing packages to the Unity package registry, so find the instructions in the `README.md` file in the internal `cesium-unity-package-registry` repo.

Once that's setup, you can upload the ZIPs to S3 with something like:

- Windows: `aws --profile cesium-ci s3 cp UE_5.8.zip s3://cesium-unreal-engine/5.8.0/`
- macOS: `aws --profile cesium-ci s3 cp UE_58-mac.zip s3://cesium-unreal-engine/5.8.0/`
- Linux: `aws --profile cesium-ci s3 cp Linux_Unreal_Engine_5.8.0.zip s3://cesium-unreal-engine/5.8.0/`

# Adding new CI jobs for the new version

The CI configuration is found in `.github/workflows/build.yml` in the `cesium-unreal` repo.

1. Copy all of the existing jobs for the previous version to the end of the file, starting with e.g., `Windows57` and continuing through to `TestPackage57`.
2. In the copied section, replace every occurrence of the previous version with the new one. The previous version may be written as something like `5.7` or it might simply be `57`.
3. Find the release notes for the new version of Unreal Engine. You can usually find this by searching for something like "unreal engine 5.8 release notes".
4. Update the CI jobs to the use the new minimum tool versions. This information is usually found in a section titled "Platform SDK Upgrades" in the release notes.
   - Prefer the tool versions specified in the "IDE Version the Build farm compiles against" section.
   - If not specified there, we should target the _minimum_ versions, not the recommended ones. By doing this, we allow our users to do the same.
   - We've sometimes found that the stated minimum is not accurate, and Unreal Engine headers do not compile with the specified version. When this happens, it's ok to use the recommended version instead. We should also report it to Epic.
   - There are some comments in the build.yml that may provide further guidance.
   - The Linux clang-version comes from the filename (minux the extension) of the link in the "Native Toolchain" column on the [Linux Version History](https://dev.epicgames.com/documentation/unreal-engine/linux-development-requirements-for-unreal-engine#version-history) section of Unreal's Linux Development Requirements page. It's pretty common that Epic doesn't update this right away after new releases, though. In that case, it's usually ok to continue to use the old Clang version from the previous UE release. And we should report this to Epic. It may also be possible to guess the correct URL from the established pattern and the information in the release notes.

Push that the changed `build.yml` file to a branch and the build will run.
