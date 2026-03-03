import fs from "fs";
import { BaseCheck } from "./base-check.js";

export class CrlfCheck extends BaseCheck {
  constructor(repoRoot, options = {}) {
    super(repoRoot, options);
  }

  get name() {
    return "CRLF";
  }

  lint(file) {
    try {
      const content = fs.readFileSync(file);
      if (content.includes("\r\n")) {
        return { status: "fail", output: "contains CRLF line endings" };
      }
      return { status: "pass" };
    } catch (err) {
      return { status: "error", output: err.message };
    }
  }

  fix(file) {
    try {
      const before = fs.readFileSync(file);
      if (before.includes("\r\n")) {
        const fixed = before.toString("utf-8").replace(/\r\n/g, "\n");
        fs.writeFileSync(file, Buffer.from(fixed, "utf-8"));
        return { status: "fixed" };
      }
      return { status: "pass" };
    } catch (err) {
      return { status: "error", output: err.message };
    }
  }
}
