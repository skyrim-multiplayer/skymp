const { execSync } = require("child_process");
const simpleGit = require("simple-git");

const extensions = [".cpp", ".h", ".hpp", ".cxx", ".cc"];

(async () => {
  try {
    const git = simpleGit();
    const changedFiles = await git.diff(["--name-only", "--cached"]);

    const filesToFormat = changedFiles
      .split("\n")
      .filter(
        (file) =>
          extensions.some((ext) => file.endsWith(ext)) && file.trim() !== ""
      );

    if (filesToFormat.length === 0) {
      console.log("No files to format.");
      process.exit(0);
    }

    console.log("Formatting files:");
    filesToFormat.forEach((file) => {
      console.log(`  - ${file}`);
      execSync(`clang-format -i ${file}`, { stdio: "inherit" });
      execSync(`git add ${file}`);
    });

    console.log("Formatting completed and changes staged.");
  } catch (err) {
    console.error("Error during formatting:", err.message);
    console.error("Autofix failed. Commit aborted.");
    process.exit(1);
  }
})();
