#!/usr/bin/env node

/*
 * Extracts a single release section from CHANGES.md.
 * Used by the Draft Release workflow to build the GitHub release notes.
 */

const fs = require("fs");
const path = require("path");

function usage() {
  console.log(`Usage:
  node scripts/changelog.js --version <X.Y.Z> [--changes-file <path>]
  npm run release-notes -- --version <X.Y.Z> [--changes-file <path>]

Required:
  --version         Release version to extract, for example 2.30.0 (no v)

Optional:
  --changes-file    Path to the changelog. Defaults to CHANGES.md in the repository root.

Prints the changelog section for the requested version, including its heading, to stdout.
`);
}

function parseArgs(argv) {
  const args = {};

  for (let index = 2; index < argv.length; ++index) {
    const value = argv[index];

    if (value === "--help" || value === "-h") {
      args.help = true;
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

function escapeRegExp(value) {
  return value.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}

function isRealDate(value) {
  if (!/^\d{4}-\d{2}-\d{2}$/.test(value)) {
    return false;
  }

  const parsed = new Date(`${value}T00:00:00Z`);
  return !Number.isNaN(parsed.getTime()) && parsed.toISOString().startsWith(value);
}

function extractReleaseSection(contents, version) {
  const lines = contents.split(/\r?\n/);
  const heading = new RegExp(`^### v${escapeRegExp(version)} - (.+?)\\s*$`);

  let start = -1;
  let date = "";

  for (let index = 0; index < lines.length; ++index) {
    const match = heading.exec(lines[index]);
    if (match) {
      start = index;
      date = match[1];
      break;
    }
  }

  if (start === -1) {
    throw new Error(
      `Could not find a "### v${version} - <date>" heading in the changelog. Has the release section been added yet?`
    );
  }

  if (!isRealDate(date)) {
    throw new Error(
      `The v${version} changelog heading has an unresolved release date: "${date}". Expected YYYY-MM-DD.`
    );
  }

  let end = lines.length;
  for (let index = start + 1; index < lines.length; ++index) {
    if (lines[index].startsWith("### ")) {
      end = index;
      break;
    }
  }

  const body = lines.slice(start + 1, end).join("\n").trim();
  if (body === "") {
    throw new Error(`The v${version} changelog section is empty.`);
  }

  return `${lines[start]}\n\n${body}`;
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

  const changesPath = args.changesFile
    ? path.resolve(args.changesFile)
    : path.resolve(__dirname, "..", "CHANGES.md");

  const contents = fs.readFileSync(changesPath, "utf8");

  process.stdout.write(`${extractReleaseSection(contents, args.version)}\n`);
}

module.exports = { extractReleaseSection };

if (require.main === module) {
  try {
    main();
  } catch (error) {
    console.error(error instanceof Error ? error.message : error);
    process.exit(1);
  }
}
