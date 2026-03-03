import fs from "fs";
import path from "path";
import simpleGit from "simple-git";
import { BaseFileSource } from "./base-file-source.js";

/**
 * Returns files currently staged in git (--cached).
 * Typical use: pre-commit hook.
 */
export class StagedFilesSource extends BaseFileSource {
  get name() {
    return "Staged files";
  }

  async resolve() {
    const git = simpleGit(this.repoRoot);
    const output = await git.diff(["--name-only", "--cached"]);
    return output
      .split("\n")
      .filter((f) => f.trim() !== "")
      .map((f) => path.resolve(this.repoRoot, f))
      .filter((f) => fs.existsSync(f));
  }
}
