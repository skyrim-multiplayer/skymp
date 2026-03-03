import path from "path";
import fs from "fs/promises";

/**
 * @typedef {"pass" | "fail" | "fixed" | "error"} CheckStatus
 */

/**
 * @typedef {Object} CheckResult
 * @property {CheckStatus} status  - Outcome of the check.
 * @property {string}      [output] - Optional diagnostic text (diff, error message, etc.).
 */

/**
 * Base class for all linter checks.
 * Subclasses must implement: name, checkDeps, lint, fix.
 *
 * appliesTo() is provided by BaseCheck using options from config:
 *   options.extensions   - array of extensions to include (e.g. [".cpp", ".h"])
 *   options.excludePaths - array of path substrings to skip
 *   options.textOnly     - if true, skip binary files (default: false)
 *
 * All methods that touch files are async.
 * appliesTo(), lint() and fix() return Promises.
 *
 * Checks must NOT write to stdout/stderr directly.
 */
export class BaseCheck {
  #extensions;
  #excludePaths;
  #textOnly;

  constructor(repoRoot, options = {}) {
    this.repoRoot = repoRoot;
    this.#extensions = (options.extensions || []).map((e) => e.toLowerCase());
    this.#excludePaths = options.excludePaths || [];
    this.#textOnly = options.textOnly ?? false;
  }

  /**
   * @returns {string} Human-readable name of the check.
   */
  get name() {
    throw new Error("Not implemented: name");
  }

  /**
   * Whether this check's dependencies are satisfied.
   * @param {object} deps - Resolved dependencies (e.g. { clangFormatPath }).
   * @returns {boolean}
   */
  checkDeps(deps) {
    return true;
  }

  /**
   * Whether this check applies to the given file.
   * Uses config-driven extensions, excludePaths, and textOnly.
   * Subclasses can override for extra logic but should await super.appliesTo().
   * @param {string} file - Absolute path to the file.
   * @returns {Promise<boolean>}
   */
  async appliesTo(file) {
    // excludePaths check
    for (const p of this.#excludePaths) {
      if (file.includes(p)) return false;
    }

    // extensions filter (empty = all extensions allowed)
    if (this.#extensions.length > 0) {
      const ext = path.extname(file).toLowerCase();
      if (!this.#extensions.includes(ext)) return false;
    }

    // binary file detection
    if (this.#textOnly) {
      let fh;
      try {
        fh = await fs.open(file, "r");
        const buffer = Buffer.alloc(1024);
        const { bytesRead } = await fh.read(buffer, 0, 1024, 0);
        for (let i = 0; i < bytesRead; i++) {
          if (buffer[i] === 0) return false;
        }
      } catch {
        return false;
      } finally {
        if (fh) await fh.close();
      }
    }

    return true;
  }

  /**
   * Lint (read-only check) a single file.
   * @param {string} file - Absolute path.
   * @param {object} deps - Resolved dependencies.
   * @returns {Promise<CheckResult>}
   */
  async lint(file, deps) {
    throw new Error("Not implemented: lint");
  }

  /**
   * Fix (in-place modify) a single file.
   * @param {string} file - Absolute path.
   * @param {object} deps - Resolved dependencies.
   * @returns {Promise<CheckResult>}
   */
  async fix(file, deps) {
    throw new Error("Not implemented: fix");
  }
}
