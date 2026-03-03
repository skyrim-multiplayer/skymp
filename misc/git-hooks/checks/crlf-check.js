import fs from "fs/promises";
import { BaseCheck } from "./base-check.js";

export class CrlfCheck extends BaseCheck {
  constructor(repoRoot, options = {}) {
    super(repoRoot, options);
  }

  get name() {
    return "CRLF";
  }

  async lint(file) {
    try {
      const content = await fs.readFile(file);
      if (content.includes("\r\n")) {
        return { status: "fail", output: "contains CRLF line endings" };
      }
      return { status: "pass" };
    } catch (err) {
      return { status: "error", output: err.message };
    }
  }

  async fix(file) {
    try {
      const before = await fs.readFile(file);
      if (before.includes("\r\n")) {
        const fixed = before.toString("utf-8").replace(/\r\n/g, "\n");
        await fs.writeFile(file, Buffer.from(fixed, "utf-8"));
        return { status: "fixed" };
      }
      return { status: "pass" };
    } catch (err) {
      return { status: "error", output: err.message };
    }
  }
}
