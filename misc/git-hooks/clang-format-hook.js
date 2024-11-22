const { execSync } = require("child_process");
const simpleGit = require("simple-git");
const fs = require("fs");
const path = require("path");

const extensions = [".cpp", ".h", ".hpp", ".cxx", ".cc"];

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

const formatFiles = (files) => {
  const filesToFormat = files.filter((file) =>
    extensions.some((ext) => file.endsWith(ext))
  );

  if (filesToFormat.length === 0) {
    console.log("No files to format.");
    return;
  }

  console.log("Formatting files:");
  filesToFormat.forEach((file) => {
    console.log(`  - ${file}`);
    execSync(`clang-format -i ${file}`, { stdio: "inherit" });
  });
};

(async () => {
  const args = process.argv.slice(2);

  try {
    if (args.includes("--all")) {
      console.log("Formatting all files in the repository...");
      const allFiles = findFiles(process.cwd());
      formatFiles(allFiles);
    } else {
      console.log("Formatting staged files...");
      const git = simpleGit();
      const changedFiles = await git.diff(["--name-only", "--cached"]);

      const filesToFormat = changedFiles
        .split("\n")
        .filter((file) => file.trim() !== "");
      formatFiles(filesToFormat);

      filesToFormat.forEach((file) => execSync(`git add ${file}`));
    }

    console.log("Formatting completed.");
  } catch (err) {
    console.error("Error during formatting:", err.message);
    process.exit(1);
  }
})();
