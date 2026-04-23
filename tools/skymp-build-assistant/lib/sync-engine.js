import crypto from 'crypto';
import fs from 'fs';
import path from 'path';

import { artifactNeedsSkyrim } from './manifest.js';

function sha256File(filePath) {
  const buf = fs.readFileSync(filePath);
  return crypto.createHash('sha256').update(buf).digest('hex');
}

function hashOrNull(filePath) {
  try {
    if (!fs.existsSync(filePath)) {
      return null;
    }
    return sha256File(filePath);
  } catch {
    return null;
  }
}

/**
 * @typedef {'ok' | 'dirty' | 'missing_source' | 'missing_dest' | 'skipped' | 'skipped_no_skyrim'} SyncState
 */

/**
 * Compare file hashes (sync-dev-runtime.ps1 semantics).
 */
export function compareFile(source, dest) {
  const sh = hashOrNull(source);
  const dh = hashOrNull(dest);
  if (sh == null) {
    return { state: 'missing_source', sourceHash: null, destHash: dh };
  }
  if (dh == null) {
    return { state: 'missing_dest', sourceHash: sh, destHash: null };
  }
  if (sh === dh) {
    return { state: 'ok', sourceHash: sh, destHash: dh };
  }
  return { state: 'dirty', sourceHash: sh, destHash: dh };
}

function listFilesRecursive(dir) {
  /** @type {string[]} */
  const out = [];
  function walk(d) {
    const entries = fs.readdirSync(d, { withFileTypes: true });
    for (const ent of entries) {
      const full = path.join(d, ent.name);
      if (ent.isDirectory()) {
        walk(full);
      } else if (ent.isFile()) {
        out.push(full);
      }
    }
  }
  walk(dir);
  return out;
}

export function compareDirectory(sourceDir, destDir) {
  if (!fs.existsSync(sourceDir)) {
    return { state: 'missing_source', updated: 0, unchanged: 0, missingDestFiles: 0 };
  }
  const sourceFiles = listFilesRecursive(sourceDir);
  if (sourceFiles.length === 0) {
    return { state: 'skipped', updated: 0, unchanged: 0, missingDestFiles: 0 };
  }
  let updated = 0;
  let unchanged = 0;
  let missingDestFiles = 0;
  for (const sf of sourceFiles) {
    const rel = sf.slice(sourceDir.length).replace(/^[/\\]/, '');
    const df = path.join(destDir, rel);
    const cmp = compareFile(sf, df);
    if (cmp.state === 'missing_dest') {
      missingDestFiles++;
      updated++;
    } else if (cmp.state === 'dirty') {
      updated++;
    } else if (cmp.state === 'ok') {
      unchanged++;
    }
  }
  return { state: updated === 0 ? 'ok' : 'dirty', updated, unchanged, missingDestFiles };
}

export function syncFile(source, dest, dryRun = false) {
  const cmp = compareFile(source, dest);
  if (cmp.state === 'missing_source') {
    return { copied: false, cmp };
  }
  if (cmp.state === 'ok') {
    return { copied: false, cmp };
  }
  if (!dryRun) {
    fs.mkdirSync(path.dirname(dest), { recursive: true });
    fs.copyFileSync(source, dest);
  }
  return { copied: true, cmp };
}

export function syncDirectoryContents(sourceDir, destDir, dryRun = false) {
  const cmp = compareDirectory(sourceDir, destDir);
  if (cmp.state === 'missing_source' || cmp.state === 'skipped') {
    return { copiedFiles: 0, cmp };
  }
  let copiedFiles = 0;
  const sourceFiles = listFilesRecursive(sourceDir);
  for (const sf of sourceFiles) {
    const rel = sf.slice(sourceDir.length).replace(/^[/\\]/, '');
    const df = path.join(destDir, rel);
    const fileCmp = compareFile(sf, df);
    if (fileCmp.state === 'ok') {
      continue;
    }
    if (!dryRun) {
      fs.mkdirSync(path.dirname(df), { recursive: true });
      fs.copyFileSync(sf, df);
    }
    copiedFiles++;
  }
  return {
    copiedFiles,
    cmp: {
      ...cmp,
      state: copiedFiles ? 'dirty' : 'ok',
    },
  };
}

export function removeStaleFiles(sourceDir, destDir, relativePaths, dryRun = false) {
  /** @type {{ path: string; removed: boolean }[]} */
  const actions = [];
  for (const rel of relativePaths) {
    const srcPath = path.join(sourceDir, rel);
    const dstPath = path.join(destDir, rel);
    if (fs.existsSync(srcPath)) {
      continue;
    }
    if (!fs.existsSync(dstPath)) {
      continue;
    }
    if (!dryRun) {
      fs.unlinkSync(dstPath);
    }
    actions.push({ path: dstPath, removed: true });
  }
  return actions;
}

/**
 * Flatten manifest artifact into sync operations.
 */
export function evaluateArtifact(artifact, ctx) {
  const needsSkyrim = artifactNeedsSkyrim(artifact);
  const skipGame = needsSkyrim && !ctx.skyrimRoot;

  /** @type {any[]} */
  const pairs = [];
  const kind = artifact.kind || 'file';
  const optional = artifact.optional === true;

  const sources = artifact.sources ?? [];
  const destinations = artifact.destinations ?? [];

  if (sources.length === 0 || destinations.length === 0) {
    return pairs;
  }

  for (const src of sources) {
    for (const dst of destinations) {
      pairs.push({
        artifactId: artifact.id,
        label: artifact.label ?? artifact.id,
        producer: artifact.producer,
        kind,
        optional,
        skipGame,
        sourcePath: src.path,
        sourceLabel: src.label,
        destPath: dst.path,
        destLabel: dst.label,
      });
    }
  }
  return pairs;
}

export function pairStatus(pair, dryRun = false) {
  if (pair.skipGame) {
    return {
      pair,
      status: 'skipped_no_skyrim',
      detail: 'Set skyrimRoot in .skymp-dev.yaml or SKYRIM_DIR',
    };
  }

  if (pair.kind === 'directory') {
    if (!fs.existsSync(pair.sourcePath)) {
      return {
        pair,
        status: pair.optional ? 'skipped' : 'missing_source',
        detail: null,
      };
    }
    const cmp = compareDirectory(pair.sourcePath, pair.destPath);
    return {
      pair,
      status: cmp.state === 'ok' ? 'ok' : 'dirty',
      detail: cmp,
    };
  }

  const cmp = compareFile(pair.sourcePath, pair.destPath);
  if (cmp.state === 'missing_source') {
    return {
      pair,
      status: pair.optional ? 'skipped' : 'missing_source',
      detail: cmp,
    };
  }
  return {
    pair,
    status: cmp.state === 'ok' ? 'ok' : 'dirty',
    detail: cmp,
  };
}

export function syncPair(pair, dryRun = false) {
  if (pair.skipGame) {
    return { synced: false, reason: 'skipped_no_skyrim' };
  }
  if (pair.kind === 'directory') {
    if (!fs.existsSync(pair.sourcePath)) {
      return { synced: false, reason: pair.optional ? 'skipped' : 'missing_source' };
    }
    const r = syncDirectoryContents(pair.sourcePath, pair.destPath, dryRun);
    return { synced: r.copiedFiles > 0, ...r };
  }
  const r = syncFile(pair.sourcePath, pair.destPath, dryRun);
  return { synced: r.copied, ...r };
}
