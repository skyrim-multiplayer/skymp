import fs from "fs";
import path from "path";
import https from "https";
import crypto from "crypto";
import { exec, spawnSync } from "child_process";
import os from "os";
import { fileURLToPath } from "url";
import Stream from "stream";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// Download artifacts go into the gitignored tools/ directory
const TOOLS_DIR = path.join(__dirname, "..", "tools");
export const CACHE_PATH = path.join(TOOLS_DIR, "cache");
export const EXTRACTED_PATH = path.join(TOOLS_DIR, "extracted");

export function ensureDirExists(dirPath) {
  if (!fs.existsSync(dirPath)) {
    fs.mkdirSync(dirPath, { recursive: true });
  }
}

export function checkInPath(exeName) {
  const command = os.platform() === "win32" ? "where" : "which";
  try {
    const result = spawnSync(command, [exeName], { encoding: "utf8", stdio: "pipe" });
    if (result.error || result.status !== 0) {
      return null;
    }
    const foundPath = result.stdout.trim().split(os.EOL)[0];
    return foundPath || null;
  } catch {
    return null;
  }
}

/**
 * @param {Stream.Readable} stream
 * @param {string} expectedSha256
 */
function makeSha256Verifier(stream, expectedSha256) {
  const hash = crypto.createHash("sha256");
  stream.on("data", (chunk) => hash.update(chunk));
  return () =>
    new Promise((resolve, reject) => {
      stream.on("error", reject);
      const validate = () => {
        const actualHash = hash.digest("hex");
        if (actualHash.toLowerCase() !== expectedSha256.toLowerCase()) {
          reject(
            new Error(
              `SHA256 mismatch: expected ${expectedSha256}, got ${actualHash}. ` +
                `Fix expected sha or delete the file and try again`
            )
          );
          return;
        }
        resolve();
      };
      if (stream.closed) {
        validate();
      } else {
        stream.on("end", validate);
      }
    });
}

export function downloadFile(url, destPath, expectedSha256) {
  return new Promise((resolve, reject) => {
    if (fs.existsSync(destPath)) {
      console.log(`${destPath} already downloaded, validating...`);
      makeSha256Verifier(fs.createReadStream(destPath), expectedSha256)().then(resolve, reject);
      return;
    }

    const options = {
      headers: { "User-Agent": "Node.js-Dependency-Downloader" },
    };

    console.log(`Downloading from ${url}...`);
    const request = https.get(url, options, (response) => {
      if (response.statusCode === 301 || response.statusCode === 302) {
        downloadFile(response.headers.location, destPath, expectedSha256).then(resolve).catch(reject);
        return;
      }

      if (response.statusCode !== 200) {
        reject(new Error(`Failed to download '${url}'. Status: ${response.statusCode}`));
        return;
      }

      const file = fs.createWriteStream(destPath);
      response.pipe(file);

      const sha256Verifier = makeSha256Verifier(response, expectedSha256);

      file.on("finish", () => {
        file.close(() => {
          sha256Verifier().then(resolve, reject);
        });
      });

      request.on("error", (err) => {
        fs.unlink(destPath, () => {});
        reject(err);
      });
    });
  });
}

export function extractArchive(archivePath, destDir) {
  return new Promise((resolve, reject) => {
    const platform = os.platform();
    let command;

    if (platform === "win32") {
      command = `powershell -command "Expand-Archive -Path '${archivePath}' -DestinationPath '${destDir}' -Force"`;
    } else {
      ensureDirExists(destDir);
      command = `tar -xf '${archivePath}' -C '${destDir}'`;
    }

    exec(command, (error) => {
      if (error) {
        reject(error);
        return;
      }
      resolve();
    });
  });
}
