---
name: prepare-release
description: 'Prepare a Cesium for Unreal release by auditing CHANGES.md against all changes since the last release, proposing missing changelog entries, determining the release date, and then running the release-prep script.'
argument-hint: 'New release version'
user-invocable: true
---

# Prepare Release

Prepare the Cesium for Unreal release prep changes.

Inputs:
- New Cesium for Unreal version: <X.Y.Z>
- Release date: determine the closest first working day of the current or next month unless I override it.

Procedure:
1. Determine the release date as the closest first working day of the current or next month, unless I explicitly override it.
2. Find the previous release tag and review all changes since that release.
3. Check every change against CHANGES.md and identify anything missing from the changelog.
4. If anything is missing, propose the missing changelog entries before making any edits.
5. Set the pending CHANGES.md section header to the new release version and release date.
6. Update CHANGES.md so the pending release section fully represents the changes since the last release, and explicitly add the cesium-native update note.
7. Run the release-prep script directly after the changelog edits.
8. Use the script for CesiumForUnreal.uplugin, package.json, and updating extern/cesium-native by checking out `main`, pulling the latest changes, and staging the parent repo's submodule reference.
9. Review the resulting diff before committing anything.

Important constraints:
- Do not handle the ion token in this workflow.
- Do not commit anything.
- The changelog review is mandatory, and missing entries must be proposed before CHANGES.md is updated.
- Explicitly add the cesium-native update note to the release section in CHANGES.md.
- Run the release-prep script directly once the changelog edits are ready.
- The script should update extern/cesium-native by checking out `main`, pulling, and staging the submodule reference.
- The script should derive the cesium-native release version from the latest tag.
- If the selected month starts on a weekend, use the first Monday as the release date.