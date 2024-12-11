const { execSync } = require("child_process");
const simpleGit = require("simple-git");
const fs = require("fs");
const path = require("path");

/**
 * Utility: Recursively find all files in a directory.
 */
const findFiles = (dir, fileList = []) => {
  const files = fs.readdirSync(dir);
  files.forEach((file) => {
    const fullPath = path.join(dir, file);
    if (fs.statSync(fullPath).isDirectory()) {
      findFiles(fullPath, fileList);
    } else {
      fileList.push(fullPath);
    }
  });
  return fileList;
};

/**
 * Check Registry: Define custom checks here.
 * Each check should implement `lint` and `fix` methods.
 */
const checks = [
  {
    name: "Clang Format",
    appliesTo: (file) => [".cpp", ".h", ".hpp", ".cxx", ".cc"].some((ext) => file.endsWith(ext)),
    lint: (file) => {
      // Example: Use clang-format to lint
      const lintCommand = `clang-format --dry-run --Werror ${file}`;
      try {
        execSync(lintCommand, { stdio: "inherit" });
        console.log(`[PASS] ${file}`);
        return true;
      } catch (error) {
        console.error(`[FAIL] ${file}`);
        return false;
      }
    },
    fix: (file) => {
      // Use clang-format to autofix
      const fixCommand = `clang-format -i ${file}`;
      execSync(fixCommand, { stdio: "inherit" });
      console.log(`[FIXED] ${file}`);
    },
  },
  {
    name: "Header/TypeScript Pair Check",
    // Applies to files that reside in the specified parent directories
    appliesTo: (file) => {
      const serverDir = "skymp5-server/cpp/messages";
      const clientDir = "skymp5-client/src/services/messages";
      const validDirs = [serverDir, clientDir];

      // Check if the file belongs to one of the valid parent directories
      return validDirs.some((dir) => file.includes(path.sep + dir + path.sep))
        && !file.endsWith(path.sep + "anyMessage.ts");
    },
    lint: (file) => {
      const serverDir = "skymp5-server/cpp/messages";
      const clientDir = "skymp5-client/src/services/messages";
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
const runChecks = (files, { lintOnly = false }) => {
  const filesToCheck = files.filter((file) =>
    checks.some((check) => check.appliesTo(file))
  );

  if (filesToCheck.length === 0) {
    console.log("No matching files found for checks.");
    return;
  }

  console.log(`${lintOnly ? "Linting" : "Fixing"} files:`);

  let fail = false;

  filesToCheck.forEach((file) => {
    checks.forEach((check) => {
      if (check.appliesTo(file)) {
        try {
          const res = lintOnly ? check.lint(file) : check.fix(file);
          if (res === false) {
            fail = true;
          }
        } catch (err) {
          if (lintOnly) {
            console.error(`Error in ${check.name}:`, err);
            process.exit(1);
          }
          else {
            throw err;
          }
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
  const lintOnly = args.includes("--lint");
  const allFiles = args.includes("--all");

  try {
    let files = [];

    if (allFiles) {
      console.log("Processing all files in the repository...");
      files = findFiles(process.cwd());
    } else {
      console.log("Processing staged files...");
      const git = simpleGit();
      const changedFiles = await git.diff(["--name-only", "--cached"]);
      files = changedFiles
        .split("\n")
        .filter((file) => file.trim() !== "")
        .filter((file) => fs.existsSync(file)); // Exclude deleted files
    }

    runChecks(files, { lintOnly });

    if (!lintOnly && !allFiles) {
      files.forEach((file) => execSync(`git add ${file}`));
    }
  } catch (err) {
    console.error("Error during processing:", err.message);
    process.exit(1);
  }
})();
