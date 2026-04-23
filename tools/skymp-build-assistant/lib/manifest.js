import fs from 'fs';
import YAML from 'yaml';

import { expandDeep } from './expand.js';

/**
 * @returns {boolean} true if this artifact touches Skyrim install paths (needs SKYRIM_DIR).
 */
export function artifactNeedsSkyrim(artifact) {
  if (artifact.requiresSkyrim === true) {
    return true;
  }
  const blob = JSON.stringify(artifact.destinations ?? []);
  return blob.includes('${skyrimPlatformDir}') || blob.includes('${skyrimDataDir}');
}

export function loadManifest(filePath) {
  const raw = fs.readFileSync(filePath, 'utf8');
  /** @type {any} */
  const doc = YAML.parse(raw);
  if (!doc || typeof doc !== 'object') {
    throw new Error(`Invalid manifest: ${filePath}`);
  }
  if (doc.version !== 1) {
    throw new Error(`Unsupported manifest version: ${doc.version} (${filePath})`);
  }
  return doc;
}

export function applyManifestVariables(manifestDoc, ctx) {
  const vars = manifestDoc.variables ?? {};
  const expandedVars = expandDeep(vars, ctx);
  const merged = { ...expandedVars, ...ctx };
  return expandDeep(manifestDoc, merged);
}
