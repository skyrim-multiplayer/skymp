import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import simpleGit from "simple-git";
import { spawnSync } from "child_process";
import { getClangFormatPath, getLinelintPath } from "./deps.js";
import { ensureCleanExit } from "./util.js";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const VALID_MODES = ["hook", "gha", "manual"];

const getRepoRoot = () => {
  const result = spawnSync("git", ["rev-parse", "--show-toplevel"], {
    encoding: "utf-8",
  });
  if (result.error || result.status !== 0) {
    console.warn("Warning: not a git repository, using cwd as repo root");
    return process.cwd();
  }
  return result.stdout.trim();
};

const REPO_ROOT = getRepoRoot();

/**
 * Load checks from linter-config.json, filtered by mode.
 * Each entry in the config specifies a module path and export name.
 */
const loadChecks = async (mode) => {
  const configPath = path.join(__dirname, "linter-config.json");
  const config = JSON.parse(fs.readFileSync(configPath, "utf-8"));

  const checks = [];
  for (const entry of config.checks) {
    if (!entry.runIn.includes(mode)) {
      console.log(`Skipping check "${entry.name}": not enabled for mode "${mode}"`);
      continue;
    }

    const mod = await import(entry.module);
    const CheckClass = mod[entry.export];
    if (!CheckClass) {
      throw new Error(
        `Export "${entry.export}" not found in module "${entry.module}"`
      );
    }
    checks.push(new CheckClass(REPO_ROOT));
  }
  return checks;
};

/**
 * Core: Run checks (lint or fix) on given files.
 * Checks return { status, output } — caller handles all formatting.
 */
const runChecks = (files, checks, { lintOnly = false, clangFormatPath, linelintPath }) => {
  const deps = { clangFormatPath, linelintPath };

  const filesToCheck = files.filter((file) =>
    checks.some((check) => check.appliesTo(file))
  );

  if (filesToCheck.length === 0) {
    console.log("No matching files found for checks.");
    return;
  }

  console.log(`${lintOnly ? "Linting" : "Fixing"} files:`);

  let fail = false;

  checks.forEach((check) => {
    if (!check.checkDeps(deps)) {
      console.warn(`Skipped ${check.name}: failed deps check`);
      return;
    }
    filesToCheck.forEach((file) => {
      if (!check.appliesTo(file)) {
        return;
      }
      try {
        const res = lintOnly ? check.lint(file, deps) : check.fix(file, deps);
        const tag = `[${check.name}]`;

        switch (res.status) {
          case "pass":
            console.log(`[PASS] ${tag} ${file}`);
            break;
          case "fixed":
            console.log(`[FIXED] ${tag} ${file}`);
            break;
          case "fail":
            console.error(`[FAIL] ${tag} ${file}`);
            if (res.output) {
              console.error(`  ${res.output}`);
            }
            fail = true;
            break;
          case "error":
            console.error(`[ERROR] ${tag} ${file}`);
            if (res.output) {
              console.error(`  ${res.output}`);
            }
            fail = true;
            break;
          default:
            console.error(`[UNKNOWN] ${tag} ${file}: unexpected status "${res.status}"`);
            fail = true;
        }
      } catch (err) {
        console.error(`[ERROR] [${check.name}] ${file}: ${err.message}`);
        fail = true;
      }
    });
  });

  if (fail) {
    process.exit(1);
  }

  console.log(`${lintOnly ? "Linting" : "Fixing"} completed.`);
};

/**
 * CLI Entry Point
 *
 * Flags:
 *   --lint           Run checks in read-only mode (exit 1 on failure)
 *   --fix            Run checks in fix mode (modify files in-place)
 *   --all            Process all tracked files (default: staged only)
 *   --pr-diff <base> Process only files changed vs <base> branch
 *   --add            Stage fixed files with git add (requires --fix)
 *   --no-download    Do not download clang-format if missing
 *   --no-path        Do not search for clang-format in PATH
 *   --mode <mode>    Execution mode: hook | gha | manual (default: manual)
 */
(async () => {
  const args = process.argv.slice(2);
  const shouldLint = args.includes("--lint");
  const shouldFix = args.includes("--fix");
  const allFiles = args.includes("--all");
  const prDiffIndex = args.indexOf("--pr-diff");
  const prDiffBase = prDiffIndex !== -1 && args[prDiffIndex + 1] ? args[prDiffIndex + 1] : null;
  const shouldAdd = args.includes("--add");
  const shouldDownload = !args.includes("--no-download");
  const shouldSearchInPath = !args.includes("--no-path");

  const modeIndex = args.indexOf("--mode");
  const mode = modeIndex !== -1 && args[modeIndex + 1] ? args[modeIndex + 1] : "manual";

  if (!VALID_MODES.includes(mode)) {
    console.error(`Invalid mode "${mode}". Must be one of: ${VALID_MODES.join(", ")}`);
    process.exit(1);
  }

  if (!shouldLint && !shouldFix) {
    console.error("Either --lint or --fix must be specified");
    process.exit(1);
  }
  if (!shouldFix && shouldAdd) {
    console.error("--add makes no sense without --fix");
    process.exit(1);
  }

  try {
    const checks = await loadChecks(mode);

    if (checks.length === 0) {
      console.log(`No checks enabled for mode "${mode}".`);
      process.exit(0);
    }

    console.log(`Mode: ${mode} | Checks: ${checks.map((c) => c.name).join(", ")}`);

    const clangFormatPath = await getClangFormatPath({
      shouldDownload,
      shouldSearchInPath,
    });
    const linelintPath = await getLinelintPath({
      shouldDownload,
      shouldSearchInPath,
    });

    let files = [];

    if (prDiffBase) {
      console.log(`Processing files changed vs ${prDiffBase}...`);
      const git = simpleGit(REPO_ROOT);
      const diffOutput = await git.diff(["--name-only", "--diff-filter=ACMR", prDiffBase]);
      files = diffOutput
        .split("\n")
        .filter((file) => file.trim() !== "")
        .map((file) => path.resolve(REPO_ROOT, file))
        .filter((file) => fs.existsSync(file));
    } else if (allFiles) {
      console.log(
        "Processing all files in the repository (respecting .gitignore)..."
      );
      const git = simpleGit(REPO_ROOT);
      const trackedFiles = await git.raw(["ls-files"]);
      files = trackedFiles
        .split("\n")
        .filter((file) => file.trim() !== "")
        .map((file) => path.resolve(REPO_ROOT, file))
        .filter((file) => fs.existsSync(file));
    } else {
      console.log("Processing staged files...");
      const git = simpleGit(REPO_ROOT);
      const changedFiles = await git.diff(["--name-only", "--cached"]);
      files = changedFiles
        .split("\n")
        .filter((file) => file.trim() !== "")
        .filter((file) => fs.existsSync(file)); // Exclude deleted files
    }

    const startTime = Date.now();
    runChecks(files, checks, { lintOnly: shouldLint, clangFormatPath, linelintPath });
    const elapsedMs = Date.now() - startTime;
    const minutes = Math.floor(elapsedMs / 60000);
    const seconds = ((elapsedMs % 60000) / 1000).toFixed(2);
    const timeStr =
      minutes > 0
        ? `${minutes} minutes, ${seconds} seconds`
        : `${seconds} seconds`;
    console.log(`Completed in ${timeStr}`);

    if (files.length === 0) {
      console.log("No files were processed.");
      process.exit(0);
    }

    if (!shouldFix) {
      process.exit(0);
    }

    if (shouldAdd) {
      files.forEach((file) =>
        ensureCleanExit(spawnSync("git", ["add", file], { stdio: "inherit" }))
      );
    } else {
      console.log(
        "Files were not staged (use --add to stage automatically)."
      );
    }
  } catch (err) {
    console.error("Error during processing:", err.message);
    process.exit(1);
  }
})();
