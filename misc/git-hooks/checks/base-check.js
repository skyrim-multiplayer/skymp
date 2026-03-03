/**
 * Base class for all linter checks.
 * Subclasses must implement: name, checkDeps, appliesTo, lint, fix.
 */
export class BaseCheck {
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
   * @returns {boolean} true if passed, false if failed.
   */
  lint(file, deps) {
    throw new Error("Not implemented: lint");
  }

  /**
   * Fix (in-place modify) a single file.
   * @param {string} file - Absolute path.
   * @param {object} deps - Resolved dependencies.
   * @returns {boolean|void} false if fix failed.
   */
  fix(file, deps) {
    throw new Error("Not implemented: fix");
  }
}
