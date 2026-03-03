import fs from "fs";
import path from "path";
import { spawnSync } from "child_process";
import { BaseCheck } from "./base-check.js";

export class ClangFormatCheck extends BaseCheck {
  #repoRoot;

  /**
   * @param {string} repoRoot - Absolute path to the repository root.
   */
  constructor(repoRoot) {
    super();
    this.#repoRoot = repoRoot;
  }

  get name() {
    return "Clang Format";
  }

  checkDeps(deps) {
    return deps.clangFormatPath !== undefined;
  }

  appliesTo(file) {
    if (file.startsWith(path.join(this.#repoRoot, "overlay_ports"))) {
      return false;
    }
    return [".cpp", ".h", ".hpp", ".cxx", ".cc"].some((ext) =>
      file.endsWith(ext)
    );
  }

  lint(file, deps) {
    const result = spawnSync(deps.clangFormatPath, ["--dry-run", "--Werror", file], {
      stdio: "inherit",
    });

    if (result.error || result.status !== 0) {
      console.error(`[FAIL] ${file}`);
      return false;
    }

    console.log(`[PASS] ${file}`);
    return true;
  }

  fix(file, deps) {
    const before = fs.readFileSync(file);
    const result = spawnSync(deps.clangFormatPath, ["-i", file], {
      stdio: "inherit",
    });

    if (result.error || result.status !== 0) {
      console.error(`[FAIL] ${file}`);
      return false;
    }

    const after = fs.readFileSync(file);
    if (!before.equals(after)) {
      console.log(`[FIXED] ${file}`);
    } else {
      console.log(`[PASS] ${file}`);
    }
  }
}
