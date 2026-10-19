#!/usr/bin/env node

const fs = require('fs');
const path = require('path');

const localesDir = path.resolve(__dirname, '..', 'src', 'locales');
const baseLocale = 'en';
const localesToCheck = ['ru', 'de', 'es', 'fr', 'it'];

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, 'utf8'));
}

function flattenKeys(obj, parent = '', out = new Set()) {
  if (obj === null || typeof obj !== 'object' || Array.isArray(obj)) {
    return out;
  }

  for (const key of Object.keys(obj)) {
    const next = parent ? `${parent}.${key}` : key;
    out.add(next);
    flattenKeys(obj[key], next, out);
  }

  return out;
}

function diffKeys(baseSet, targetSet) {
  const missing = [];
  const extra = [];

  for (const key of baseSet) {
    if (!targetSet.has(key)) {
      missing.push(key);
    }
  }

  for (const key of targetSet) {
    if (!baseSet.has(key)) {
      extra.push(key);
    }
  }

  return {
    missing: missing.sort(),
    extra: extra.sort(),
  };
}

function printList(title, list) {
  console.error(`  ${title} (${list.length}):`);
  for (const item of list.slice(0, 30)) {
    console.error(`    - ${item}`);
  }
  if (list.length > 30) {
    console.error(`    ... and ${list.length - 30} more`);
  }
}

function run() {
  const basePath = path.join(localesDir, `${baseLocale}.json`);
  const baseJson = readJson(basePath);
  const baseKeys = flattenKeys(baseJson);

  let failed = false;

  for (const locale of localesToCheck) {
    const localePath = path.join(localesDir, `${locale}.json`);
    const localeJson = readJson(localePath);
    const localeKeys = flattenKeys(localeJson);
    const { missing, extra } = diffKeys(baseKeys, localeKeys);

    if (missing.length === 0 && extra.length === 0) {
      console.log(
        `[OK] ${locale}.json matches ${baseLocale}.json key structure`,
      );
      continue;
    }

    failed = true;
    console.error(
      `[FAIL] ${locale}.json key mismatch against ${baseLocale}.json`,
    );
    if (missing.length > 0) {
      printList('Missing keys', missing);
    }
    if (extra.length > 0) {
      printList('Extra keys', extra);
    }
  }

  if (failed) {
    process.exit(1);
  }

  console.log('Locale consistency test passed');
}

run();
