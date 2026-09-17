#!/usr/bin/env node

/*
 * Release Preparation Steps
 * - Updates extern/cesium-native
 *   update extern/cesium-native by checking out main, pulling, and staging the
 *   parent repo submodule reference.
 * - Updates CesiumForUnreal.uplugin and package.json to the target release
 *   version.
 */

const fs = require("fs");
const path = require("path");
const { execFileSync } = require("child_process");

function usage() {
  console.log(`Usage:
  node scripts/release-prep.js --version <X.Y.Z> [--dry-run]

Required:
  --version         New Cesium for Unreal version, for example 2.30.0

Optional:
  --dry-run         Print planned changes without writing files or staging the submodule.

The script derives the cesium-native version from the latest git tag in extern/cesium-native.
It updates CesiumForUnreal.uplugin and package.json, updates extern/cesium-native to main,
and leaves CHANGES.md untouched.
`);
}

function parseArgs(argv) {
  const args = { dryRun: false };

  for (let index = 2; index < argv.length; ++index) {
    const value = argv[index];

    if (value === "--help" || value === "-h") {
      args.help = true;
      continue;
    }

    if (value === "--dry-run") {
      args.dryRun = true;
      continue;
    }

    if (!value.startsWith("--")) {
      throw new Error(`Unexpected argument: ${value}`);
    }

    const key = value.slice(2).replace(/-([a-z])/g, (_, letter) => letter.toUpperCase());
    const next = argv[++index];
    if (next === undefined || next.startsWith("--")) {
      throw new Error(`Missing value for --${key}`);
    }

    args[key] = next;
  }

  return args;
}

function readText(filePath) {
  return fs.readFileSync(filePath, "utf8");
}

function writeText(filePath, contents) {
  const existing = readText(filePath);
  if (existing === contents) {
    return false;
  }

  fs.writeFileSync(filePath, contents);

  return true;
}

function latestCesiumNativeVersion() {
  // Mirrors the release-process step that checks which cesium-native version the release will reference.
  const output = execFileSync(
    "git",
    ["ls-remote", "--tags", "--refs", "https://github.com/CesiumGS/cesium-native.git", "v*"],
    { encoding: "utf8" }
  ).trim();

  if (!output) {
    throw new Error("Could not determine the latest cesium-native tag from the remote repository.");
  }

  const versions = output
    .split(/\r?\n/)
    .map((line) => line.split("\t")[1])
    .map((tag) => /^refs\/tags\/v(\d+\.\d+\.\d+)$/.exec(tag))
    .filter(Boolean)
    .map((match) => match[1])
    .sort(compareSemver);

  if (versions.length === 0) {
    throw new Error("Could not find any semantic version tags in the cesium-native remote repository.");
  }

  return versions[versions.length - 1];
}

function previousCesiumNativeVersionFromChangelog(contents) {
  const match = contents.match(/from v(\d+\.\d+\.\d+) to v(\d+\.\d+\.\d+)\./);
  if (!match) {
    throw new Error("Could not find an existing cesium-native version note in CHANGES.md.");
  }

  return match[2];
}

function compareSemver(left, right) {
  const leftParts = left.split(".").map(Number);
  const rightParts = right.split(".").map(Number);

  for (let index = 0; index < 3; ++index) {
    if (leftParts[index] !== rightParts[index]) {
      return leftParts[index] - rightParts[index];
    }
  }

  return 0;
}

function replaceJsonField(contents, fieldName, value) {
  const pattern = new RegExp(`("${fieldName}"\\s*:\\s*")([^"]+)(")`);
  if (!pattern.test(contents)) {
    throw new Error(`Could not find ${fieldName} in file.`);
  }

  return contents.replace(pattern, `$1${value}$3`);
}

function replaceJsonNumberField(contents, fieldName, value) {
  const pattern = new RegExp(`("${fieldName}"\\s*:\\s*)(\\d+)`);
  if (!pattern.test(contents)) {
    throw new Error(`Could not find ${fieldName} in file.`);
  }

  return contents.replace(pattern, `$1${value}`);
}

function updateUplugin(contents, version, versionName) {
  // Maps to the release-process step that updates CesiumForUnreal.uplugin.
  let next = contents;
  next = replaceJsonNumberField(next, "Version", version);
  next = replaceJsonField(next, "VersionName", versionName);
  return next;
}

function updatePackageJson(contents, version) {
  // Keeps package.json aligned with the release version during prep.
  return replaceJsonField(contents, "version", version);
}

function runGit(cwd, args, dryRun) {
  if (dryRun) {
    return;
  }

  execFileSync("git", ["-C", cwd, ...args], { stdio: "inherit" });
}

function main() {
  const args = parseArgs(process.argv);
  if (args.help) {
    usage();
    return;
  }

  if (!args.version) {
    throw new Error("Missing required --version argument.");
  }

  if (!/^\d+\.\d+\.\d+$/.test(args.version)) {
    throw new Error(`Invalid version format: ${args.version}`);
  }

  const repoRoot = path.resolve(__dirname, "..");
  const upluginPath = path.join(repoRoot, "CesiumForUnreal.uplugin");
  const packageJsonPath = path.join(repoRoot, "package.json");
  const cesiumNativePath = path.join(repoRoot, "extern", "cesium-native");

  const uplugin = readText(upluginPath);
  const packageJson = readText(packageJsonPath);
  const changes = readText(path.join(repoRoot, "CHANGES.md"));
  const parsedUplugin = JSON.parse(uplugin);
  const parsedPackageJson = JSON.parse(packageJson);
  const currentVersionName = parsedUplugin.VersionName;
  const currentPackageVersion = parsedPackageJson.version;
  const cesiumNativeTo = latestCesiumNativeVersion();
  const cesiumNativeFrom = previousCesiumNativeVersionFromChangelog(changes);

  // Maps to the release-process version bump for CesiumForUnreal.uplugin.
  const nextUpluginVersion = Number(parsedUplugin.Version) + 1;

  console.log(`Release prep plan for v${args.version}`);
  console.log(`- Cesium Native: v${cesiumNativeFrom} -> v${cesiumNativeTo}`);
  if (cesiumNativeTo === cesiumNativeFrom) {
    console.log("- WARNING: the latest cesium-native tag matches the changelog baseline; make sure the new native release tag has been pushed before using this script for a real release.");
  }
  console.log(`- CesiumForUnreal.uplugin: Version ${parsedUplugin.Version} -> ${nextUpluginVersion}`);
  console.log(`- CesiumForUnreal.uplugin: VersionName ${currentVersionName} -> ${args.version}`);
  console.log(`- package.json: version ${currentPackageVersion} -> ${args.version}`);
  console.log(`- extern/cesium-native: git checkout main && git pull`);
  console.log(`- extern/cesium-native: stage the updated submodule reference in the parent repo`);
  console.log(`- CHANGES.md: no scripted changes; update it separately in the release workflow`);

  if (args.dryRun) {
    return;
  }

  const nextUplugin = updateUplugin(uplugin, nextUpluginVersion, args.version);
  const nextPackageJson = updatePackageJson(packageJson, args.version);

  writeText(upluginPath, nextUplugin);
  writeText(packageJsonPath, nextPackageJson);

  // Mirrors the release-process submodule step: checkout main, pull, then stage the parent repo pointer.
  runGit(cesiumNativePath, ["checkout", "main"], false);
  runGit(cesiumNativePath, ["pull"], false);
  runGit(repoRoot, ["add", "extern/cesium-native"], false);
}

try {
  main();
} catch (error) {
  console.error(error instanceof Error ? error.message : error);
  process.exit(1);
}