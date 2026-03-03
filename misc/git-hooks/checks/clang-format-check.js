import { promises as fs } from "fs";
import { execFile } from "child_process";
import { promisify } from "util";
import { BaseCheck } from "./base-check.js";

const execFileAsync = promisify(execFile);

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

  async lint(file, deps) {
    try {
      await execFileAsync(deps.clangFormatPath, ["--dry-run", "--Werror", file]);
      return { status: "pass" };
    } catch (err) {
      if (err.code === "ENOENT") {
        return { status: "error", output: err.message };
      }
      const output = (err.stderr || err.stdout || "").toString().trim();
      return { status: "fail", output };
    }
  }

  async fix(file, deps) {
    let before;
    try {
      before = await fs.readFile(file);
    } catch (err) {
      return { status: "error", output: err.message };
    }

    try {
      await execFileAsync(deps.clangFormatPath, ["-i", file]);
    } catch (err) {
      if (err.code === "ENOENT") {
        return { status: "error", output: err.message };
      }
      const output = (err.stderr || err.stdout || "").toString().trim();
      return { status: "error", output };
    }

    try {
      const after = await fs.readFile(file);
      if (!before.equals(after)) {
        return { status: "fixed" };
      }
      return { status: "pass" };
    } catch (err) {
      return { status: "error", output: err.message };
    }
  }
}
