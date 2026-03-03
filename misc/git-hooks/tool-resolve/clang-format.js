import fs from "fs";
import path from "path";
import os from "os";
import { spawnSync } from "child_process";
import {
  CACHE_PATH,
  EXTRACTED_PATH,
  ensureDirExists,
  checkInPath,
  downloadFile,
  extractArchive,
} from "./tool-utils.js";

const VERSION = "21.1.8";

function checkVersion(exePath) {
  try {
    const child = spawnSync(exePath, ["--version"], { encoding: "utf-8", stdio: "pipe" });
    if (child.error || child.status !== 0) return "unknown";
    const match = child.stdout.match(/\bclang-format\s+version\s+([0-9]+(?:\.[0-9]+)*)\b/i);
    return match ? match[1] : "unknown";
  } catch {
    return "unknown";
  }
}

/**
 * Resolve clang-format binary path.
 * @param {{ shouldDownload: boolean, shouldSearchInPath: boolean }} options
 * @returns {Promise<string|undefined>} Path to binary, or undefined if unavailable.
 */
export async function getClangFormatPath({ shouldDownload, shouldSearchInPath }) {
  const exeName = os.platform() === "win32" ? "clang-format.exe" : "clang-format";

  if (shouldSearchInPath) {
    const systemPath = checkInPath(exeName);
    if (systemPath) {
      const systemVersion = checkVersion(systemPath);
      const systemMajor = parseInt(systemVersion.split(".")[0], 10);
      const requiredMajor = parseInt(VERSION.split(".")[0], 10);
      if (systemMajor >= requiredMajor) {
        console.log(`Using ${systemPath} from system path (version ${systemVersion})`);
        return systemPath;
      }
      console.log(
        `System clang-format is version ${systemVersion}, need ${requiredMajor}+. Will download ${VERSION}.`
      );
    } else {
      console.log(`${exeName} not found in PATH`);
    }
  }

  if (!shouldDownload) {
    console.warn("clang-format not found and downloading is disabled");
    return undefined;
  }

  const platform = os.platform();
  let url = "";
  let archiveName = "";
  let archiveSha256 = "";
  let archivePathToClangFormat = "";

  if (platform === "linux") {
    url = `https://github.com/llvm/llvm-project/releases/download/llvmorg-${VERSION}/LLVM-${VERSION}-Linux-X64.tar.xz`;
    archiveName = `LLVM-${VERSION}-Linux-X64.tar.xz`;
    archiveSha256 = "b3b7f2801d15d50736acea3c73982994d025b01c2f035b91ae3b49d1b575732b";
    archivePathToClangFormat = `LLVM-${VERSION}-Linux-X64/bin/clang-format`;
  } else {
    console.warn(`Platform ${platform} not supported for clang-format download`);
    return undefined;
  }

  ensureDirExists(CACHE_PATH);
  ensureDirExists(EXTRACTED_PATH);

  const archivePath = path.join(CACHE_PATH, archiveName);
  const extractDir = path.join(EXTRACTED_PATH, `llvm-${VERSION}`);
  const expectedExe = path.join(extractDir, archivePathToClangFormat);

  if (fs.existsSync(expectedExe)) {
    console.log(`Using downloaded ${expectedExe}, version ${checkVersion(expectedExe)}`);
    return expectedExe;
  }

  await downloadFile(url, archivePath, archiveSha256);

  ensureDirExists(extractDir);
  console.log(`Extracting clang-format from ${archiveName} (single binary, not full LLVM)...`);
  await extractArchive(archivePath, extractDir, [archivePathToClangFormat]);

  if (fs.existsSync(expectedExe)) {
    console.log(`Using downloaded ${expectedExe}, version ${checkVersion(expectedExe)}`);
    return expectedExe;
  }

  console.warn("Could not find clang-format binary after extraction");
  return undefined;
}
