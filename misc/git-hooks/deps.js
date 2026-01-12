import fs from 'fs';
import path from 'path';
import https from 'https';
import crypto from 'crypto';
import { exec, execSync } from 'child_process';
import os from 'os';
import { fileURLToPath } from 'url';

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
    if (foundPath) return foundPath;
  } catch (e) {
    return null;
  }
  return null;
}

function downloadFile(url, destPath) {
  return new Promise((resolve, reject) => {
    const options = {
      headers: {
        'User-Agent': 'Node.js-Dependency-Downloader',
      },
    };

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

      file.on('finish', () => {
        file.close();
        resolve();
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
  if (systemPath) return systemPath;
  
  const version = '21.1.8';
  
  const platform = os.platform();
  let url = '';
  let archiveName = '';

  if (platform === 'linux') {
      url = `https://github.com/llvm/llvm-project/releases/download/llvmorg-${version}/LLVM-${version}-Linux-X64.tar.xz`;
      archiveName = `LLVM-${version}-Linux-X64.tar.xz`;
  } else {
      throw new Error(`Platform ${platform} not supported by this script yet.`);
  }

  ensureDirExists(CACHE_PATH);
  ensureDirExists(EXTRACTED_PATH);

  const archivePath = path.join(CACHE_PATH, archiveName);
  const extractDir = path.join(EXTRACTED_PATH, `llvm-${version}`);
  const expectedExe = path.join(extractDir, 'bin', exeName);
  const expectedExeRoot = path.join(extractDir, exeName);

  if (fs.existsSync(expectedExe)) return expectedExe;
  if (fs.existsSync(expectedExeRoot)) return expectedExeRoot;

  if (!fs.existsSync(archivePath)) {
      console.log(`Downloading clang-format from ${url}...`);
      await downloadFile(url, archivePath);
  }
  
  ensureDirExists(extractDir);
  console.log(`Extracting ${archiveName}...`);
  await extractArchive(archivePath, extractDir);

  // Search for the binary
  if (fs.existsSync(expectedExe)) return expectedExe;
  
  // Maybe it's inside a subdir
  const subdirs = fs.readdirSync(extractDir).filter(f => fs.statSync(path.join(extractDir, f)).isDirectory());
  for (const sub of subdirs) {
      const candidate = path.join(extractDir, sub, 'bin', exeName);
      if (fs.existsSync(candidate)) return candidate;
      
      const candidateRoot = path.join(extractDir, sub, exeName);
      if (fs.existsSync(candidateRoot)) return candidateRoot;
  }
  
  throw new Error('Could not find clang-format binary after extraction');
}
