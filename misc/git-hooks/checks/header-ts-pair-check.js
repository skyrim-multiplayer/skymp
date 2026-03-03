import fs from "fs";
import path from "path";
import { BaseCheck } from "./base-check.js";

export class HeaderTsPairCheck extends BaseCheck {
  #repoRoot;

  /**
   * @param {string} repoRoot - Absolute path to the repository root.
   */
  constructor(repoRoot) {
    super();
    this.#repoRoot = repoRoot;
  }

  get name() {
    return "Header/TypeScript Pair Check";
  }

  appliesTo(file) {
    const serverDir = `${this.#repoRoot}/skymp5-server/cpp/messages`;
    const clientDir = `${this.#repoRoot}/skymp5-client/src/services/messages`;
    const validDirs = [serverDir, clientDir];

    return (
      validDirs.some(
        (dir) => file.includes(path.sep + dir + path.sep)
      ) &&
      !file.endsWith(path.sep + "anyMessage.ts") &&
      !file.endsWith(path.sep + "refrIdMessageBase.ts") &&
      !file.endsWith(path.sep + "MessageBase.h") &&
      !file.endsWith(path.sep + "MessageSerializerFactory.cpp") &&
      !file.endsWith(path.sep + "MessageSerializerFactory.h") &&
      !file.endsWith(path.sep + "Messages.h") &&
      !file.endsWith(path.sep + "MinPacketId.h") &&
      !file.endsWith(path.sep + "MsgType.h")
    );
  }

  lint(file) {
    const serverDir = `${this.#repoRoot}/skymp5-server/cpp/messages`;
    const clientDir = `${this.#repoRoot}/skymp5-client/src/services/messages`;
    const ext = path.extname(file);
    const baseName = path.basename(file, ext);

    const pairExt = ext === ".h" ? ".ts" : ".h";
    const pairDir = file.includes(path.sep + serverDir + path.sep)
      ? clientDir
      : serverDir;

    const pairFiles = fs.readdirSync(pairDir);

    const pairFile = pairFiles.find(
      (candidate) =>
        candidate.toLowerCase() === `${baseName}${pairExt}`.toLowerCase()
    );

    if (!pairFile) {
      return { status: "fail", output: `pair file not found (expected ${baseName}${pairExt} in ${pairDir})` };
    }
    return { status: "pass" };
  }

  fix() {
    // No auto-fix available for missing pair files
    return { status: "pass" };
  }
}
