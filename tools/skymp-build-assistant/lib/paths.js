import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));

/**
 * Walk upward from startDir until we find the SkyMP repo root.
 */
export function findRepoRoot(startDir = process.cwd()) {
  let dir = path.resolve(startDir);
  while (true) {
    const marker = path.join(dir, 'tools', 'skymp-build-assistant', 'manifest.default.yaml');
    const cmake = path.join(dir, 'CMakeLists.txt');
    if (fs.existsSync(marker) && fs.existsSync(cmake)) {
      return dir;
    }
    const parent = path.dirname(dir);
    if (parent === dir) {
      break;
    }
    dir = parent;
  }
  throw new Error(
    'Could not locate SkyMP repo root (need tools/skymp-build-assistant/manifest.default.yaml). ' +
      'Run from inside the repo or pass --repo <path>.',
  );
}

export function defaultManifestPath(repoRoot) {
  return path.join(repoRoot, 'tools', 'skymp-build-assistant', 'manifest.default.yaml');
}

export function defaultUserConfigPath(repoRoot) {
  return path.join(repoRoot, '.skymp-dev.yaml');
}

export function defaultDashboardStatePath(repoRoot) {
  return path.join(repoRoot, 'tools', 'skymp-build-assistant', '.skymp-dev-dashboard.json');
}

export function dashboardUiRoot() {
  return path.join(packageRoot(), 'ui');
}

export function dashboardUiDistDir() {
  return path.join(dashboardUiRoot(), 'dist');
}

export function dashboardUiIndexPath() {
  return path.join(dashboardUiDistDir(), 'index.html');
}

/** Package dir (tools/skymp-build-assistant) - for static assets */
export function packageRoot() {
  return path.resolve(__dirname, '..');
}
