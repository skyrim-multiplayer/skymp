import fs from 'fs';
import YAML from 'yaml';

import { getConfigFieldKeys, getConfigFields, getConfigSchema } from './config-schema.js';

export function loadUserConfig(configPath) {
  if (!fs.existsSync(configPath)) {
    return {};
  }
  const raw = fs.readFileSync(configPath, 'utf8');
  /** @type {Record<string, unknown>} */
  const doc = YAML.parse(raw);
  return doc && typeof doc === 'object' ? doc : {};
}

export function pickKnownConfigValues(config) {
  const keys = new Set(getConfigFieldKeys());
  return Object.fromEntries(
    Object.entries(config ?? {}).filter(([key]) => keys.has(key)),
  );
}

export function getConfigPayload(configPath) {
  const rawConfig = loadUserConfig(configPath);
  return {
    configPath,
    exists: fs.existsSync(configPath),
    schema: getConfigSchema(),
    values: pickKnownConfigValues(rawConfig),
  };
}

export function validateConfigInput(values) {
  const errors = [];
  const normalized = {};
  const providedKeys = new Set();

  for (const field of getConfigFields()) {
    if (!Object.prototype.hasOwnProperty.call(values ?? {}, field.key)) {
      continue;
    }

    providedKeys.add(field.key);
    const value = values[field.key];

    if (value == null) {
      continue;
    }

    if (typeof value !== 'string') {
      errors.push({
        field: field.key,
        message: `${field.label} must be a string.`,
      });
      continue;
    }

    const trimmed = value.trim();
    if (!trimmed) {
      continue;
    }

    normalized[field.key] = trimmed;
  }

  return {
    ok: errors.length === 0,
    errors,
    values: normalized,
    providedKeys,
  };
}

export function saveUserConfig(configPath, values) {
  const validation = validateConfigInput(values);
  if (!validation.ok) {
    const err = new Error('Validation failed');
    err.details = validation.errors;
    throw err;
  }

  const existing = loadUserConfig(configPath);
  const next = { ...existing };

  for (const key of validation.providedKeys) {
    if (Object.prototype.hasOwnProperty.call(validation.values, key)) {
      next[key] = validation.values[key];
    } else {
      delete next[key];
    }
  }

  fs.writeFileSync(configPath, YAML.stringify(next), 'utf8');
  return getConfigPayload(configPath);
}
