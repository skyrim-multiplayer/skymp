import fs from "fs";
import path from "path";
import simpleGit from "simple-git";
import { BaseFileSource } from "./base-file-source.js";

/**
 * Returns files changed compared to a base branch/ref.
 * The ref is taken from options.baseRef (config) or context.args["--pr-diff"] (CLI).
 * Typical use: CI / GitHub Actions.
 */
export class DiffBaseSource extends BaseFileSource {
  get name() {
    return "Diff vs base";
  }

  async resolve(context) {
    const baseRef = this.options.baseRef || context.cliOptions?.prDiffBase;
    if (!baseRef) {
      throw new Error("DiffBaseSource requires a baseRef (via config options.baseRef or --pr-diff CLI arg)");
    }

    const git = simpleGit(this.repoRoot);
    const output = await git.diff(["--name-only", "--diff-filter=ACMR", baseRef]);
    return output
      .split("\n")
      .filter((f) => f.trim() !== "")
      .map((f) => path.resolve(this.repoRoot, f))
      .filter((f) => fs.existsSync(f));
  }
}
