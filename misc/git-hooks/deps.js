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
  
  // 1. Check PATH (if users have it installed, preferably use it? Or prefer downloaded one for consistency?)
  // The user said "acquire clang format", suggesting we should ensure we have ours.
  // But let's check PATH first to be nice, unless version mismatches are a concern.
  // Given "repair if needed", providing a consistent binary is safer.
  // But downloading 50MB on every commit check if not present is annoying if not cached.
  // The cache logic handles it.
  
  // Let's force download/check to ensure specific version if we want consistency, 
  // or check PATH. checking PATH is faster.
  // The user says "download deps example" logic which checks PATH first.
  
  const systemPath = checkInPath(exeName);
  // We can't easily check version of system path without parsing output. 
  // Let's assume if it exists in path, it's fine, unless it fails. 
  // But the prompt implies "repair if needed", maybe the system one is missing or broken.
  if (systemPath) return systemPath;

  // 2. Download LLVM
  // We need a Linux binary.
  // Using the version from the search: LLVM 21.1.8
  // Since we might be on diverse linux, a static binary is best.
  // GitHub releases LLVM-21.1.8-Linux-X64.tar.xz
  
  const version = '21.1.8'; // Based on previous search
  // Fallback to a known stable if 21 is too new? 17.0.6 is common.
  // However, I will stick to what I saw in search results.
  
  const platform = os.platform();
  let url = '';
  let archiveName = '';
  let extractSubfolder = ''; // The folder inside the archive

  if (platform === 'linux') {
      // url = `https://github.com/llvm/llvm-project/releases/download/llvmorg-${version}/LLVM-${version}-Linux-X64.tar.xz`;
      // Search result was: https://github.com/llvm/llvm-project/releases/download/llvmorg-21.1.8/LLVM-21.1.8-Linux-X64.tar.xz
      url = `https://github.com/llvm/llvm-project/releases/download/llvmorg-${version}/LLVM-${version}-Linux-X64.tar.xz`;
      archiveName = `LLVM-${version}-Linux-X64.tar.xz`;
      // Usually LLVM archives extract to a root folder or just files. 
      // Often keys inside are bin/, lib/, etc.
      // We'll need to check structure. 
      // If it extracts to `LLVM-21.1.8-Linux-X64/`, then bin is inside.
  } else {
      throw new Error(`Platform ${platform} not supported by this script yet.`);
  }

  ensureDirExists(CACHE_PATH);
  ensureDirExists(EXTRACTED_PATH);

  const archivePath = path.join(CACHE_PATH, archiveName);
  
  // Check if already extracted
  // We identify the extracted folder by a hash or just the name. 
  // Let's use version + platform as key.
  const extractDir = path.join(EXTRACTED_PATH, `llvm-${version}`);
  const expectedExe = path.join(extractDir, 'bin', exeName);
  // Also check top level just in case
  const expectedExeRoot = path.join(extractDir, exeName);

  if (fs.existsSync(expectedExe)) return expectedExe;
  if (fs.existsSync(expectedExeRoot)) return expectedExeRoot;

  // Download
  if (!fs.existsSync(archivePath)) {
      console.log(`Downloading clang-format from ${url}...`);
      await downloadFile(url, archivePath);
  }

  // Extract
  // To avoid mess, extract to a temp dir then move? 
  // Or extract to `extractDir`.
  // If the archive contains a top-level directory, we need to strip it or find it.
  
  // Let's peek content? No, just extract to `extractDir` and search.
  // But if `tar` extracts `LLVM-.../bin/...`, then `extractDir/LLVM-.../bin/...`
  // We should extract to `EXTRACTED_PATH` and rename? 
  // Or extract to `extractDir` and if it creates a subdir, find it.
  
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
