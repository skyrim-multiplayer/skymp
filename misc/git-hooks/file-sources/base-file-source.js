/**
 * Base class for file sources.
 * A file source determines which files to process.
 */
export class BaseFileSource {
  constructor(repoRoot, options = {}) {
    this.repoRoot = repoRoot;
    this.options = options;
  }

  /**
   * @returns {string} Human-readable name of the source.
   */
  get name() {
    throw new Error("Not implemented: name");
  }

  /**
   * Resolve the list of absolute file paths to process.
   * @param {object} context - { args: string[] } CLI args for parametric sources.
   * @returns {Promise<string[]>} Absolute paths.
   */
  async resolve(context) {
    throw new Error("Not implemented: resolve");
  }
}
