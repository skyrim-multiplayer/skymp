import fs from 'fs';
import path from 'path';
import https from 'https';
import crypto from 'crypto';
import { exec, execSync } from 'child_process';
import os from 'os';
import { fileURLToPath } from 'url';
import Stream from 'stream';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const BASE_PATH = path.join(__dirname, 'tools');
const CACHE_PATH = path.join(BASE_PATH, 'cache');
const EXTRACTED_PATH = path.join(BASE_PATH, 'extracted');

function ensureDirExists(dirPath) {
  if (!fs.existsSync(dirPath)) {
    fs.mkdirSync(dirPath, { recursive: true });
  }
}

function checkInPath(exeName) {
  let command = 'where';
  if (os.platform() !== 'win32') {
    command = 'which';
  }
  try {
    const result = execSync(`${command} ${exeName}`, { encoding: 'utf8', stdio: 'pipe' });
    const foundPath = result.trim().split(os.EOL)[0];
    if (foundPath) {
      execSync(`${foundPath} --version`, { encoding: 'utf-8', stdio: 'pipe' });
      return foundPath;
    }
  } catch (e) {
    return null;
  }
  return null;
}

/**
 * @param {Stream.Readable} stream
 * @param {*} expectedSha256
 */
function makeSha256Verifier(stream, expectedSha256) {
  const hash = crypto.createHash('sha256');
  stream.on('data', (chunk) => hash.update(chunk));
  return () => new Promise((resolve, reject) => {
    const actualHash = hash.digest('hex');

    if (actualHash.toLowerCase() !== expectedSha256.toLowerCase()) {
      reject(new Error(
        `SHA256 mismatch for '${url}': expected ${expectedSha256}, got ${actualHash}. ` +
        `Fix expected sha or delete the file and try again`,
      ));
      return;
    }

    resolve();
  });
}

function downloadFile(url, destPath, expectedSha256) {
  return new Promise((resolve, reject) => {
    if (fs.existsSync(destPath)) {
      makeSha256Verifier(fs.createReadStream(destPath), expectedSha256)().then(resolve, reject);
      return;
    }

    const options = {
      headers: {
        'User-Agent': 'Node.js-Dependency-Downloader',
      },
    };

    console.log(`Downloading clang-format from ${url}...`);
    const request = https.get(url, options, (response) => {
      if (response.statusCode === 301 || response.statusCode === 302) {
        downloadFile(response.headers.location, destPath)
          .then(resolve)
          .catch(reject);
        return;
      }

      if (response.statusCode !== 200) {
        reject(new Error(`Failed to download '${url}'. Status: ${response.statusCode}`));
        return;
      }

      const file = fs.createWriteStream(destPath);
      response.pipe(file);

      const sha256Verifier = makeSha256Verifier(response, expectedSha256);

      file.on('finish', () => {
        file.close(() => {
          sha256Verifier().then(resolve, reject);
        });
      });

      request.on('error', (err) => {
        fs.unlink(destPath, () => {});
        reject(err);
      });
    });
  });
}

function extractArchive(archivePath, destDir) {
  return new Promise((resolve, reject) => {
    // console.log(`[Extract] Extracting "${archivePath}" to "${destDir}"...`);
    const platform = os.platform();
    let command;

    if (platform === 'win32') {
        command = `powershell -command "Expand-Archive -Path '${archivePath}' -DestinationPath '${destDir}' -Force"`;
    } else {
        // tar -xf works for .tar.xz if tar supports it (modern tar does)
        ensureDirExists(destDir);
        command = `tar -xf '${archivePath}' -C '${destDir}'`;
    }

    exec(command, (error, stdout, stderr) => {
      if (error) {
        reject(error);
        return;
      }
      resolve();
    });
  });
}

export async function getClangFormatPath() {
  const exeName = os.platform() === 'win32' ? 'clang-format.exe' : 'clang-format';
  
  const systemPath = checkInPath(exeName);
  if (systemPath) {
    return systemPath;
  }
  
  const version = '21.1.8';
  
  const platform = os.platform();
  let url = '';
  let archiveName = '';
  let archiveSha256 = '';

  if (platform === 'linux') {
    url = `https://github.com/llvm/llvm-project/releases/download/llvmorg-${version}/LLVM-${version}-Linux-X64.tar.xz`;
    archiveName = `LLVM-${version}-Linux-X64.tar.xz`;
    archiveSha256 = '';
  } else {
    throw new Error(`Platform ${platform} not supported by this script yet.`);
  }

  ensureDirExists(CACHE_PATH);
  ensureDirExists(EXTRACTED_PATH);

  const archivePath = path.join(CACHE_PATH, archiveName);
  const extractDir = path.join(EXTRACTED_PATH, `llvm-${version}`);
  const expectedExe = path.join(extractDir, 'bin', exeName);

  if (fs.existsSync(expectedExe)) {
    return expectedExe;
  }

  await downloadFile(url, archivePath, archiveSha256);

  ensureDirExists(extractDir);
  console.log(`Extracting ${archiveName}...`);
  await extractArchive(archivePath, extractDir);

  if (fs.existsSync(expectedExe)) {
    return expectedExe;
  }
  
  throw new Error('Could not find clang-format binary after extraction');
}
