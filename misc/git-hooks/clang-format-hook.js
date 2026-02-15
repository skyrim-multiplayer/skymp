import fs from "fs";
import path from "path";
import simpleGit from "simple-git";
import { spawnSync } from "child_process";
import { getClangFormatPath } from "./deps.js";
import { fileURLToPath } from "url";
import { ensureCleanExit } from "./util.js";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const REPO_ROOT = path.resolve(path.join(__dirname, '..', '..'));

/**
 * Utility: Recursively find all files in a directory.
 */
const findFiles = (dir) => {
  const childResult = spawnSync('git', ['ls-files', dir, '-z'], { encoding: 'utf-8' });
  if (childResult.error) {
    throw error;
  }
  if (childResult.status !== 0) {
    console.error(`Failed to list files: git exited with ${childResult.status}`);
    return [];
  }
  return childResult.stdout.split('\0').filter((file) => file.length > 0).map((file) => path.resolve(file));
};

/**
 * Check Registry: Define custom checks here.
 * Each check should implement `lint` and `fix` methods.
 */
const getChecks = () => [
  {
    name: "Clang Format",
    checkDeps: (deps) => deps.clangFormatPath !== undefined,
    appliesTo: (file) => {
      if (file.startsWith(path.join(REPO_ROOT, 'overlay_ports'))) {
        return false;
      }
      return [".cpp", ".h", ".hpp", ".cxx", ".cc"].some((ext) => file.endsWith(ext));
    },
    lint: (file, deps) => {
      const result = spawnSync(deps.clangFormatPath, ["--dry-run", "--Werror", file], { stdio: "inherit" });

      if (result.error || result.status !== 0) {
        console.error(`[FAIL] ${file}`);
        return false;
      }

      console.log(`[PASS] ${file}`);
      return true;
    },
    fix: (file, deps) => {
      const result = spawnSync(deps.clangFormatPath, ["-i", file], { stdio: "inherit" });

      if (result.error || result.status !== 0) {
        console.error(`[FAIL] ${file}`);
        return false;
      }

      console.log(`[FIXED] ${file}`);
    },
  },
  {
    name: "Header/TypeScript Pair Check",
    checkDeps: (deps) => true,
    // Applies to files that reside in the specified parent directories
    appliesTo: (file) => {
      const serverDir = `${REPO_ROOT}/skymp5-server/cpp/messages`;
      const clientDir = `${REPO_ROOT}/skymp5-client/src/services/messages`;
      const validDirs = [serverDir, clientDir];

      // Check if the file belongs to one of the valid parent directories
      return validDirs.some((dir) => file.includes(path.sep + dir + path.sep))
        && !file.endsWith(path.sep + "anyMessage.ts")
        && !file.endsWith(path.sep + "refrIdMessageBase.ts")
        && !file.endsWith(path.sep + "MessageBase.h")
        && !file.endsWith(path.sep + "MessageSerializerFactory.cpp")
        && !file.endsWith(path.sep + "MessageSerializerFactory.h")
        && !file.endsWith(path.sep + "Messages.h")
        && !file.endsWith(path.sep + "MinPacketId.h")
        && !file.endsWith(path.sep + "MsgType.h");
    },
    lint: (file) => {
      const serverDir = `${REPO_ROOT}/skymp5-server/cpp/messages`;
      const clientDir = `${REPO_ROOT}/skymp5-client/src/services/messages`;
      const ext = path.extname(file);
      const baseName = path.basename(file, ext);

      // Determine the pair file's extension and directory
      const pairExt = ext === ".h" ? ".ts" : ".h";
      const pairDir = file.includes(path.sep + serverDir + path.sep)
        ? clientDir
        : serverDir;

      const pairFiles = fs.readdirSync(pairDir);

      // Find a case-insensitive match
      const pairFile = pairFiles.find(
        (candidate) => candidate.toLowerCase() === `${baseName}${pairExt}`.toLowerCase()
      );

      if (!pairFile) {
        console.error(`[FAIL] Pair file not found for ${file}: ${pairFile}`);
        return false;
      } else {
        console.log(`[PASS] Pair file found for ${file}: ${pairFile}`);
        return true;
      }
    },
    fix() { }
  }
];

/**
 * Core: Run checks (lint or fix) on given files.
 */
const runChecks = (files, { lintOnly = false, clangFormatPath }) => {
  const checks = getChecks();
  const deps = { clangFormatPath };

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
        if (res === false) {
          fail = true;
        }
      } catch (err) {
        if (lintOnly) {
          console.error(`Error in ${check.name}:`, err);
          process.exit(1);
        } else {
          throw err;
        }
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
 */
(async () => {
  const args = process.argv.slice(2);
  const shouldLint = args.includes("--lint");
  const shouldFix = args.includes("--fix");
  const allFiles = args.includes("--all");
  const shouldAdd = args.includes("--add");
  const shouldDownload = !args.includes('--no-download');
  const shouldSearchInPath = !args.includes('--no-path');

  const clangFormatPath = await getClangFormatPath({ shouldDownload, shouldSearchInPath });

  if (!shouldLint && !shouldFix) {
    console.error('Either --lint or --fix must be specified');
    process.exit(1);
  }
  if (!shouldFix && shouldAdd) {
    console.error('--add makes no sense without --fix');
    process.exit(1);
  }

  try {
    let files = [];

    if (allFiles) {
      console.log("Processing all files in the repository...");
      files = findFiles(path.resolve(path.join(__dirname, '..', '..')));
    } else {
      console.log("Processing staged files...");
      const git = simpleGit();
      const changedFiles = await git.diff(["--name-only", "--cached"]);
      files = changedFiles
        .split("\n")
        .filter((file) => file.trim() !== "")
        .filter((file) => fs.existsSync(file)); // Exclude deleted files
    }

    runChecks(files, { lintOnly: shouldLint, clangFormatPath });

    if (files.length === 0) {
      console.log('No files were processed.');
      process.exit(0);
    }

    if (!shouldFix) {
      process.exit(0);
    }

    if (shouldAdd) {
      files.forEach((file) => ensureCleanExit(spawnSync('git', ['add', file], { stdio: 'inherit' })));
    } else {
      console.log('Files were processed, but not added. To add next time, use --add. To add this time, run:');
      console.log(`git add ${files.join(' ')}`);
    }
  } catch (err) {
    console.error("Error during processing:", err.message);
    process.exit(1);
  }
})();
