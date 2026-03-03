import { promises as fs } from "fs";
import path from "path";
import { BaseCheck } from "./base-check.js";

/**
 * Generic paired-files check.
 *
 * Configured via options in linter-config.json:
 *   dirs:    array of { path, ext } — exactly two paired directories
 *   exclude: array of filenames to skip
 *
 * For each file in dir[0], expects a file with the same basename
 * but dir[1].ext in dir[1].path, and vice versa.
 */
export class PairedFilesCheck extends BaseCheck {
  #absDirs;
  #exclude;

  constructor(repoRoot, options = {}) {
    super(repoRoot, options);

    const dirs = options.dirs || [];
    if (dirs.length !== 2) {
      throw new Error("PairedFilesCheck requires exactly 2 entries in options.dirs");
    }
    this.#absDirs = dirs.map((d) => ({
      abs: path.resolve(repoRoot, d.path),
      ext: d.ext,
    }));
    this.#exclude = new Set((options.exclude || []).map((f) => f.toLowerCase()));
  }

  get name() {
    return "Paired Files Check";
  }

  async appliesTo(file) {
    const basename = path.basename(file).toLowerCase();
    if (this.#exclude.has(basename)) return false;
    return this.#absDirs.some((d) => file.startsWith(d.abs + path.sep));
  }

  async lint(file) {
    const ext = path.extname(file);
    const baseName = path.basename(file, ext);

    const ownDir = this.#absDirs.find((d) => file.startsWith(d.abs + path.sep));
    const pairDir = this.#absDirs.find((d) => d !== ownDir);

    let pairFiles;
    try {
      pairFiles = await fs.readdir(pairDir.abs);
    } catch (err) {
      return { status: "error", output: `cannot read pair directory ${pairDir.abs}: ${err.message}` };
    }

    const expected = `${baseName}${pairDir.ext}`;
    const found = pairFiles.find(
      (c) => c.toLowerCase() === expected.toLowerCase()
    );

    if (!found) {
      return { status: "fail", output: `pair file not found (expected ${expected} in ${pairDir.abs})` };
    }
    return { status: "pass" };
  }

  async fix(file) {
    // No auto-fix — just re-run lint so caller sees the status
    return this.lint(file);
  }
}
