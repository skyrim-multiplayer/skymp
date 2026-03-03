import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import { spawnSync } from "child_process";
import pLimit from "p-limit";
import { getClangFormatPath, getLinelintPath } from "./deps.js";
import { ensureCleanExit } from "./util.js";
import { builtinRegistry } from "./registry.js";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

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
 * Resolve a class from config entry.
 * If "module" is present → dynamic import (for custom user-provided checks).
 * Otherwise → look up "export" in the built-in registry.
 */
const resolveClass = async (entry) => {
  const exportName = entry.export;
  if (entry.module) {
    const mod = await import(entry.module);
    const Cls = mod[exportName];
    if (!Cls) {
      throw new Error(`Export "${exportName}" not found in "${entry.module}"`);
    }
    return Cls;
  }
  const Cls = builtinRegistry[exportName];
  if (!Cls) {
    throw new Error(
      `Export "${exportName}" not found in built-in registry. ` +
      `Available: ${Object.keys(builtinRegistry).join(", ")}. ` +
      `For custom checks, specify "module" in config.`
    );
  }
  return Cls;
};

/**
 * Load config, instantiate file source and checks for the given mode.
 */
const loadConfig = async (mode) => {
  const configPath = path.join(REPO_ROOT, "linter-config.json");
  const config = JSON.parse(fs.readFileSync(configPath, "utf-8"));

  // --- file source ---
  const modeConfig = config.modes[mode];
  if (!modeConfig) {
    throw new Error(`Unknown mode "${mode}". Available: ${Object.keys(config.modes).join(", ")}`);
  }
  const srcEntry = modeConfig.fileSource;
  const SrcClass = await resolveClass(srcEntry);
  const fileSource = new SrcClass(REPO_ROOT, srcEntry.options || {});

  // --- checks ---
  const checks = [];
  for (const entry of config.checks) {
    if (!entry.modes.includes(mode)) {
      console.log(`Skipping check "${entry.name}": not enabled for mode "${mode}"`);
      continue;
    }
    const CheckClass = await resolveClass(entry);
    checks.push(new CheckClass(REPO_ROOT, entry.options || {}));
  }

  return { fileSource, checks };
};

/**
 * Make path relative to REPO_ROOT for compact output.
 */
const relPath = (file) => {
  if (file.startsWith(REPO_ROOT + path.sep)) {
    return file.slice(REPO_ROOT.length + 1);
  }
  return file;
};

/**
 * Format all check results for a single file into log lines.
 *
 * If every check passed  → single line: [PASS] rel/path [Check1, Check2, ...]
 * If every check fixed   → single line: [FIXED] rel/path [Check1, Check2, ...]
 * If mixed pass+fixed    → single line: [OK] rel/path [passed: A, B | fixed: C]
 * Otherwise              → one line per failed/errored check with details.
 *
 * @param {{ res: CheckResult, checkName: string }[]} results
 * @param {string} file  Absolute path.
 * @returns {{ lines: string[], isFail: boolean, stats: { pass: number, fixed: number, fail: number, error: number } }}
 */
const formatFileResults = (results, file) => {
  const rel = relPath(file);
  const lines = [];
  let isFail = false;
  const stats = { pass: 0, fixed: 0, fail: 0, error: 0 };

  const passed = [];
  const fixed = [];
  const bad = [];

  for (const { res, checkName } of results) {
    switch (res.status) {
      case "pass":
        passed.push(checkName);
        stats.pass++;
        break;
      case "fixed":
        fixed.push(checkName);
        stats.fixed++;
        break;
      case "fail":
        bad.push({ res, checkName });
        stats.fail++;
        break;
      case "error":
      default:
        bad.push({ res, checkName });
        stats.error++;
        break;
    }
  }

  if (bad.length === 0) {
    // All good — compact summary
    if (fixed.length === 0) {
      lines.push(`[PASS] ${rel} [${passed.join(", ")}]`);
    } else if (passed.length === 0) {
      lines.push(`[FIXED] ${rel} [${fixed.join(", ")}]`);
    } else {
      const parts = [];
      if (passed.length) parts.push(`passed: ${passed.join(", ")}`);
      if (fixed.length) parts.push(`fixed: ${fixed.join(", ")}`);
      lines.push(`[OK] ${rel} [${parts.join(" | ")}]`);
    }
  } else {
    // Some failures — print each result individually
    isFail = true;
    for (const name of passed) {
      lines.push(`[PASS] ${rel} [${name}]`);
    }
    for (const name of fixed) {
      lines.push(`[FIXED] ${rel} [${name}]`);
    }
    for (const { res, checkName } of bad) {
      const status = res.status === "fail" ? "FAIL" : res.status === "error" ? "ERROR" : "UNKNOWN";
      lines.push(`[${status}] ${rel} [${checkName}]`);
      if (res.output) lines.push(`  ${res.output}`);
    }
  }

  return { lines, isFail, stats };
};

/**
 * Core: Run checks (lint or fix) on given files.
 *
 * Lint mode:  all (check, file) pairs run in parallel.
 * Fix mode:   one file at a time (sequential) to avoid races on shared files.
 */
const runChecks = async (files, checks, { lintOnly = false, verbose = false, clangFormatPath, linelintPath }) => {
  const deps = { clangFormatPath, linelintPath };

  // Group checks by file instead of a sequential flat array
  const fileToChecks = new Map();
  let totalChecks = 0;

  for (const check of checks) {
    if (!check.checkDeps(deps)) {
      console.warn(`Skipped ${check.name}: failed deps check`);
      continue;
    }
    for (const file of files) {
      if (await check.appliesTo(file)) {
        if (!fileToChecks.has(file)) {
          fileToChecks.set(file, []);
        }
        fileToChecks.get(file).push(check);
        totalChecks++;
      }
    }
  }

  const groupedWork = Array.from(fileToChecks.entries()).map(([file, fileChecks]) => ({ file, checks: fileChecks }));

  if (groupedWork.length === 0) {
    console.log("No matching files found for checks.");
    return;
  }

  console.log(`${lintOnly ? "Linting" : "Fixing"} ${totalChecks} check(s) across ${groupedWork.length} file(s)...`);

  let fail = false;
  const counters = { pass: 0, fixed: 0, fail: 0, error: 0 };

  if (lintOnly) {
    // Parallel lint: controlled by p-limit per file
    const limit = pLimit(10); // reasonable default for lints
    await Promise.all(
      groupedWork.map(({ file, checks }) =>
        limit(async () => {
          // Run all checks for this file in parallel
          const results = await Promise.all(
            checks.map(async (check) => {
              try {
                const res = await check.lint(file, deps);
                return { res, checkName: check.name };
              } catch (err) {
                return { res: { status: "error", output: err.message }, checkName: check.name };
              }
            })
          );

          const { lines, isFail, stats } = formatFileResults(results, file);
          counters.pass += stats.pass;
          counters.fixed += stats.fixed;
          counters.fail += stats.fail;
          counters.error += stats.error;
          if (lines.length > 0) {
            if (isFail) {
              console.error(lines.join("\n"));
            } else if (verbose) {
              console.log(lines.join("\n"));
            }
          }
          if (isFail) fail = true;
        })
      )
    );
  } else {
    // Sequential fix: file by file, check by check to avoid file races
    for (const { file, checks } of groupedWork) {
      const fileResults = [];

      for (const check of checks) {
        try {
          const res = await check.fix(file, deps);
          fileResults.push({ res, checkName: check.name });
        } catch (err) {
          fileResults.push({ res: { status: "error", output: err.message }, checkName: check.name });
        }
      }

      const { lines, isFail, stats } = formatFileResults(fileResults, file);
      counters.pass += stats.pass;
      counters.fixed += stats.fixed;
      counters.fail += stats.fail;
      counters.error += stats.error;
      if (lines.length > 0) {
        if (isFail) {
          console.error(lines.join("\n"));
        } else if (verbose) {
          console.log(lines.join("\n"));
        }
      }
      if (isFail) fail = true;
    }
  }

  // Summary
  const parts = [`${totalChecks} check(s)`];
  if (counters.pass > 0) parts.push(`${counters.pass} passed`);
  if (counters.fixed > 0) parts.push(`${counters.fixed} fixed`);
  if (counters.fail > 0) parts.push(`${counters.fail} failed`);
  if (counters.error > 0) parts.push(`${counters.error} errored`);
  console.log(`Summary: ${parts.join(", ")}`);

  if (fail) {
    process.exit(1);
  }

  console.log(`${lintOnly ? "Linting" : "Fixing"} completed.`);
};

/**
 * CLI Entry Point
 *
 * Flags:
 *   --verbose        Show [PASS] lines (hidden by default)
 *   --lint           Run checks in read-only mode (exit 1 on failure)
 *   --fix            Run checks in fix mode (modify files in-place)
 *   --add            Stage fixed files with git add (requires --fix)
 *   --no-download    Do not download tools if missing
 *   --no-path        Do not search for tools in PATH
 *   --mode <mode>    Execution mode (key in config.modes, default: manual)
 */
(async () => {
  const args = process.argv.slice(2);
  const shouldLint = args.includes("--lint");
  const shouldFix = args.includes("--fix");
  const shouldAdd = args.includes("--add");
  const verbose = args.includes("--verbose");
  const shouldDownload = !args.includes("--no-download");
  const shouldSearchInPath = !args.includes("--no-path");

  const modeIndex = args.indexOf("--mode");
  const mode = modeIndex !== -1 && args[modeIndex + 1] ? args[modeIndex + 1] : "manual";

  if (!shouldLint && !shouldFix) {
    console.error("Either --lint or --fix must be specified");
    process.exit(1);
  }
  if (!shouldFix && shouldAdd) {
    console.error("--add makes no sense without --fix");
    process.exit(1);
  }

  try {
    const { fileSource, checks } = await loadConfig(mode);

    if (checks.length === 0) {
      console.log(`No checks enabled for mode "${mode}".`);
      process.exit(0);
    }

    console.log(`Mode: ${mode} | Source: ${fileSource.name} | Checks: ${checks.map((c) => c.name).join(", ")}`);

    const clangFormatPath = await getClangFormatPath({
      shouldDownload,
      shouldSearchInPath,
    });
    const linelintPath = await getLinelintPath({
      shouldDownload,
      shouldSearchInPath,
    });

    const files = await fileSource.resolve();
    console.log(`${fileSource.name}: ${files.length} file(s)`);

    const startTime = Date.now();
    await runChecks(files, checks, { lintOnly: shouldLint, verbose, clangFormatPath, linelintPath });
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
