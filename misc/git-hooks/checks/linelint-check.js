import { spawnSync } from "child_process";
import { BaseCheck } from "./base-check.js";
import fs from "fs";

export class LinelintCheck extends BaseCheck {
  constructor(repoRoot, options = {}) {
    super(repoRoot, options);
  }

  get name() {
    return "Linelint";
  }

  checkDeps(deps) {
    return deps.linelintPath !== undefined;
  }

  lint(file, deps) {
    const result = spawnSync(deps.linelintPath, [file], { cwd: this.repoRoot, stdio: "pipe" });
    if (result.error) {
      return { status: "error", output: result.error.message };
    }
    if (result.status !== 0) {
      const out = (result.stderr || result.stdout || "").toString().trim();
      return { status: "fail", output: out || "linelint failed" };
    }
    return { status: "pass" };
  }

  fix(file, deps) {
    let before;
    try {
      before = fs.readFileSync(file);
    } catch (err) {
      return { status: "error", output: err.message };
    }

    const result = spawnSync(deps.linelintPath, ["-a", file], { cwd: this.repoRoot, stdio: "pipe" });
    if (result.error) {
      return { status: "error", output: result.error.message };
    }

    try {
      const after = fs.readFileSync(file);
      if (!before.equals(after)) {
        return { status: "fixed" };
      }
      return { status: "pass" };
    } catch (err) {
      return { status: "error", output: err.message };
    }
  }
}
