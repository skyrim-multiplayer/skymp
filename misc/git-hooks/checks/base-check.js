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
 * Subclasses must implement: name, checkDeps, appliesTo, lint, fix.
 *
 * lint() and fix() must return a CheckResult object:
 *   { status: "pass" | "fail" | "fixed" | "error", output?: string }
 *
 * Checks must NOT write to stdout/stderr directly.
 */
export class BaseCheck {
  constructor(repoRoot) {
    this.repoRoot = repoRoot;
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
   * @param {string} file - Absolute path to the file.
   * @returns {boolean}
   */
  appliesTo(file) {
    return false;
  }

  /**
   * Lint (read-only check) a single file.
   * @param {string} file - Absolute path.
   * @param {object} deps - Resolved dependencies.
   * @returns {CheckResult}
   */
  lint(file, deps) {
    throw new Error("Not implemented: lint");
  }

  /**
   * Fix (in-place modify) a single file.
   * @param {string} file - Absolute path.
   * @param {object} deps - Resolved dependencies.
   * @returns {CheckResult}
   */
  fix(file, deps) {
    throw new Error("Not implemented: fix");
  }
}
