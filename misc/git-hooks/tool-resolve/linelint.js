import fs from "fs";
import path from "path";
import os from "os";
import {
  CACHE_PATH,
  ensureDirExists,
  checkInPath,
  downloadFile,
} from "./tool-utils.js";

const VERSION = "0.0.6";

/**
 * Resolve linelint binary path.
 * @param {{ shouldDownload: boolean, shouldSearchInPath: boolean }} options
 * @returns {Promise<string|undefined>} Path to binary, or undefined if unavailable.
 */
export async function getLinelintPath({ shouldDownload, shouldSearchInPath }) {
  const exeName = os.platform() === "win32" ? "linelint.exe" : "linelint";

  if (shouldSearchInPath) {
    const systemPath = checkInPath(exeName);
    if (systemPath) {
      console.log(`Using ${systemPath} from system path instead of downloading`);
      return systemPath;
    }
    console.log(`${exeName} not found in PATH`);
  }

  if (!shouldDownload) {
    console.warn("linelint not found and downloading is disabled");
    return undefined;
  }

  const platform = os.platform();
  let url = "";
  let exeSha256 = "";

  if (platform === "linux") {
    url = `https://github.com/fernandrone/linelint/releases/download/${VERSION}/linelint-linux-amd64`;
    exeSha256 = "16b70fb7b471d6f95cbdc0b4e5dc2b0ac9e84ba9ecdc488f7bdf13df823aca4b";
  } else if (platform === "win32") {
    url = `https://github.com/fernandrone/linelint/releases/download/${VERSION}/linelint-windows-amd64`;
    exeSha256 = "69793b89716c4a3ed02ff95d922ef95e0224bb987c938e2f8e85af1c79820bf3";
  } else if (platform === "darwin") {
    url = `https://github.com/fernandrone/linelint/releases/download/${VERSION}/linelint-darwin-amd64`;
    exeSha256 = "2c6264704ea0479666ce2be7140e84c74f6fef8e7e9d9203db9d8bf8ca438e84";
  } else {
    console.warn(`Platform ${platform} not supported for linelint download`);
    return undefined;
  }

  ensureDirExists(CACHE_PATH);

  const destPath = path.join(CACHE_PATH, exeName);

  if (fs.existsSync(destPath)) {
    console.log(`Using cached ${destPath}`);
    return destPath;
  }

  console.log(`Downloading linelint v${VERSION}...`);
  await downloadFile(url, destPath, exeSha256);

  if (platform !== "win32") {
    fs.chmodSync(destPath, 0o755);
  }

  if (fs.existsSync(destPath)) {
    console.log(`Using downloaded ${destPath}`);
    return destPath;
  }

  console.warn("Could not find linelint binary after download");
  return undefined;
}
