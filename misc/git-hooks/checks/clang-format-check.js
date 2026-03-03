import fs from "fs";
import { spawnSync } from "child_process";
import { BaseCheck } from "./base-check.js";

export class ClangFormatCheck extends BaseCheck {
  constructor(repoRoot, options = {}) {
    super(repoRoot, options);
  }

  get name() {
    return "Clang Format";
  }

  checkDeps(deps) {
    return deps.clangFormatPath !== undefined;
  }

  lint(file, deps) {
    const result = spawnSync(deps.clangFormatPath, ["--dry-run", "--Werror", file], {
      stdio: "pipe",
    });

    if (result.error) {
      return { status: "error", output: result.error.message };
    }

    if (result.status !== 0) {
      const output = (result.stderr || result.stdout || "").toString().trim();
      return { status: "fail", output };
    }

    return { status: "pass" };
  }

  fix(file, deps) {
    const before = fs.readFileSync(file);
    const result = spawnSync(deps.clangFormatPath, ["-i", file], {
      stdio: "pipe",
    });

    if (result.error) {
      return { status: "error", output: result.error.message };
    }

    if (result.status !== 0) {
      const output = (result.stderr || result.stdout || "").toString().trim();
      return { status: "error", output };
    }

    const after = fs.readFileSync(file);
    if (!before.equals(after)) {
      return { status: "fixed" };
    }
    return { status: "pass" };
  }
}
