import fs from "fs";
import path from "path";
import simpleGit from "simple-git";
import { BaseFileSource } from "./base-file-source.js";

/**
 * Returns all tracked files in the repo.
 * Typical use: manual full-repo check.
 */
export class AllFilesSource extends BaseFileSource {
  get name() {
    return "All tracked files";
  }

  async resolve() {
    const git = simpleGit(this.repoRoot);
    const output = await git.raw(["ls-files"]);
    return output
      .split("\n")
      .filter((f) => f.trim() !== "")
      .map((f) => path.resolve(this.repoRoot, f))
      .filter((f) => fs.existsSync(f));
  }
}
