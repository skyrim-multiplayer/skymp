import { spawnSync } from "child_process";
import { BaseCheck } from "./base-check.js";
import path from "path";
import fs from "fs";

export class LinelintCheck extends BaseCheck {
  get name() {
    return "Linelint & CRLF";
  }

  checkDeps(deps) {
    return deps.linelintPath !== undefined;
  }

  appliesTo(file) {
    if (file.includes("/third_party/") || file.includes("\\third_party\\") || file.includes("/skymp5-scripts/")) return false;
    if (file.includes("/overlay_ports/rsm-bsa/") && file.endsWith(".patch")) return false;
    if (file.includes("/overlay_ports/libxml2/")) return false;

    // Skip known binary extensions
    const binExts = [
      ".png", ".jpg", ".jpeg", ".gif", ".ico", ".wav", ".mp3", ".ogg",
      ".mp4", ".avi", ".ttf", ".woff", ".woff2", ".eot", ".swf", ".pex",
      ".bin", ".exe", ".dll", ".so", ".dylib", ".zip", ".tar", ".gz", ".7z",
      ".DS_Store"
    ];
    const ext = path.extname(file).toLowerCase();
    if (binExts.includes(ext)) return false;

    // Optional: check if it's text by reading the first 1KB
    try {
      const fd = fs.openSync(file, 'r');
      const buffer = Buffer.alloc(1024);
      const bytesRead = fs.readSync(fd, buffer, 0, 1024, 0);
      fs.closeSync(fd);
      for (let i = 0; i < bytesRead; i++) {
        if (buffer[i] === 0) return false;
      }
      return true;
    } catch (err) {
      return false;
    }
  }

  lint(file, deps) {
    let fail = false;
    // 1. CRLF check
    try {
      const content = fs.readFileSync(file);
      if (content.includes('\r\n')) {
        console.error(`[FAIL] ${file} contains CRLF line endings`);
        fail = true;
      }
    } catch (err) {
      console.error(`[ERROR] ${file}: ${err.message}`);
      return false;
    }

    // 2. linelint check
    const result = spawnSync(deps.linelintPath, [file], { cwd: this.repoRoot, stdio: "inherit" });
    if (result.error || result.status !== 0) {
      console.error(`[FAIL] linelint failed on ${file}`);
      fail = true;
    }

    if (!fail) {
      console.log(`[PASS] ${file}`);
    }

    return !fail;
  }

  fix(file, deps) {
    let before;
    try {
      before = fs.readFileSync(file);
    } catch (err) {
      return false;
    }

    // 1. Fix CRLF
    try {
      if (before.includes('\r\n')) {
        const str = before.toString('utf-8').replace(/\r\n/g, '\n');
        fs.writeFileSync(file, Buffer.from(str, 'utf-8'));
      }
    } catch (err) {
      console.error(`[ERROR] ${file}: ${err.message}`);
      return false;
    }

    // 2. linelint fix
    spawnSync(deps.linelintPath, ["-a", file], { cwd: this.repoRoot, stdio: "pipe" });

    try {
      const after = fs.readFileSync(file);
      if (!before.equals(after)) {
        console.log(`[FIXED] ${file}`);
      } else {
        console.log(`[PASS] ${file}`);
      }
    } catch (err) {
      return false;
    }
  }
}
