import fs from 'fs';
import path from 'path';

import { loadUserConfig } from './config.js';
import { buildContext } from './context.js';
import { applyManifestVariables, loadManifest } from './manifest.js';
import {
  evaluateArtifact,
  pairStatus,
  removeStaleFiles,
  syncPair,
} from './sync-engine.js';

/**
 * Load expanded manifest + evaluation context.
 */
export function loadWorkspace({ repoRoot, manifestPath, configPath }) {
  const manifestDoc = loadManifest(manifestPath);
  const config = loadUserConfig(configPath);
  const ctx = buildContext({
    repoRoot,
    config,
  });
  const expanded = applyManifestVariables(manifestDoc, ctx);
  return { manifestDoc: expanded, ctx, config };
}

export function collectPairs(manifestDoc) {
  /** @type {any[]} */
  const pairs = [];
  for (const art of manifestDoc.artifacts ?? []) {
    pairs.push(...evaluateArtifact(art, manifestDoc.__ctx ?? {}));
  }
  return pairs;
}

/**
 * Collect pairs with evaluation context attached on manifest doc for skipGame logic.
 */
export function collectPairsWithContext(manifestDoc, ctx) {
  /** @type {any[]} */
  const pairs = [];
  for (const art of manifestDoc.artifacts ?? []) {
    pairs.push(...evaluateArtifact(art, ctx));
  }
  return pairs;
}

export function runStatus(manifestDoc, ctx) {
  const pairs = collectPairsWithContext(manifestDoc, ctx);
  return pairs.map((p) => ({
    ...pairStatus(p, true),
    pair: p,
  }));
}

export function runSync(manifestDoc, ctx, { dryRun = false, onlyId = null } = {}) {
  const pairs = collectPairsWithContext(manifestDoc, ctx);
  const filter = onlyId ? pairs.filter((p) => p.artifactId === onlyId) : pairs;
  /** @type {any[]} */
  const results = [];
  for (const p of filter) {
    const st = pairStatus(p, true);
    if (st.status === 'ok') {
      results.push({ pair: p, result: { skipped: true, reason: 'ok' } });
      continue;
    }
    if (st.status === 'skipped_no_skyrim') {
      results.push({ pair: p, result: { skipped: true, reason: 'skipped_no_skyrim' } });
      continue;
    }
    if (st.status === 'skipped') {
      results.push({ pair: p, result: { skipped: true, reason: 'skipped' } });
      continue;
    }
    if (st.status === 'missing_source') {
      results.push({
        pair: p,
        result: {
          skipped: true,
          reason: 'missing_source',
          optional: p.optional === true,
        },
      });
      continue;
    }
    results.push({ pair: p, result: syncPair(p, dryRun) });
  }

  /** @type {any[]} */
  const stale = [];
  for (const rule of manifestDoc.staleRemoval ?? []) {
    const sourceDir = rule.sourceDirectory;
    const destDir = rule.destinationDirectory;
    const rels = rule.relativePaths ?? [];
    if (!ctx.skyrimRoot) {
      continue;
    }
    const actions = removeStaleFiles(sourceDir, destDir, rels, dryRun);
    if (actions.length) {
      stale.push({ rule: rule.id, actions });
    }
  }
  return { results, stale };
}

export function watchPathsForManifest(_manifestDoc, ctx) {
  const buildDir = ctx.buildDir;
  /** @type {string[]} */
  const paths = [];
  const pushIf = (p) => {
    if (fs.existsSync(p)) {
      paths.push(p);
    }
  };
  pushIf(path.join(buildDir, 'dist', 'client'));
  pushIf(path.join(buildDir, 'skyrim-platform', '_platform_se', 'bin'));
  pushIf(path.join(ctx.repoRoot, 'skymp5-gamemode', 'gamemode.js'));
  if (ctx.nirnLabOutputDir) {
    pushIf(ctx.nirnLabOutputDir);
  }
  if (paths.length === 0) {
    console.warn(
      '[skymp-dev watch] Nothing to watch yet - build once so build/dist/client exists.',
    );
  }
  return paths;
}
