import fs from 'fs';
import path from 'path';

const PLUGIN_EXTENSIONS = new Set(['.esp', '.esm', '.esl']);
const DEFAULT_HEADER_LINES = [
  '# This file is used by Skyrim to keep track of your downloaded content.',
  '# Please do not modify this file.',
];

function createCommentLine(raw) {
  return { type: 'comment', raw };
}

function createBlankLine() {
  return { type: 'blank' };
}

function createPluginLine(name, enabled) {
  return {
    type: 'plugin',
    name,
    enabled: enabled === true,
  };
}

function pluginKey(value) {
  return String(value || '').trim().toLowerCase();
}

export function isPluginFilename(value) {
  const extension = path.extname(String(value || '')).toLowerCase();
  return PLUGIN_EXTENSIONS.has(extension);
}

export function createEmptyPluginsTxtDocument(pluginsTxtPath) {
  return finalizePluginsTxtDocument({
    path: pluginsTxtPath || '',
    exists: false,
    readable: true,
    lineEnding: '\r\n',
    lines: DEFAULT_HEADER_LINES.map((line) => createCommentLine(line)),
  });
}

function finalizePluginsTxtDocument(document) {
  const occurrencesByName = new Map();
  const orderedPluginNames = [];
  const pluginOrderByName = new Map();

  for (const line of document.lines) {
    if (line.type !== 'plugin') {
      continue;
    }

    const key = pluginKey(line.name);
    if (!pluginOrderByName.has(key)) {
      pluginOrderByName.set(key, orderedPluginNames.length);
      orderedPluginNames.push(line.name);
    }
    if (!occurrencesByName.has(key)) {
      occurrencesByName.set(key, []);
    }
    occurrencesByName.get(key).push({
      name: line.name,
      enabled: line.enabled === true,
    });
  }

  const pluginStateByName = new Map();
  const duplicatePluginNames = [];
  const conflictingPluginNames = [];

  for (const [key, occurrences] of occurrencesByName.entries()) {
    const enabled = occurrences.some((entry) => entry.enabled === true);
    const duplicate = occurrences.length > 1;
    const conflicting = duplicate && occurrences.some((entry) => entry.enabled !== enabled);

    pluginStateByName.set(key, {
      name: occurrences[0]?.name ?? key,
      enabled,
      duplicate,
      conflicting,
      occurrences,
    });

    if (duplicate) {
      duplicatePluginNames.push(occurrences[0]?.name ?? key);
    }
    if (conflicting) {
      conflictingPluginNames.push(occurrences[0]?.name ?? key);
    }
  }

  return {
    ...document,
    duplicatePluginNames,
    conflictingPluginNames,
    orderedPluginNames,
    pluginOrderByName,
    pluginStateByName,
  };
}

function parsePluginsTxtLine(rawLine) {
  const trimmed = rawLine.trim();
  if (!trimmed) {
    return createBlankLine();
  }

  if (trimmed.startsWith('#') || trimmed.startsWith(';')) {
    return createCommentLine(rawLine);
  }

  let enabled = false;
  let pluginName = trimmed;
  if (pluginName.startsWith('*')) {
    enabled = true;
    pluginName = pluginName.slice(1).trim();
  }

  if (!isPluginFilename(pluginName)) {
    return createCommentLine(rawLine);
  }

  return createPluginLine(pluginName, enabled);
}

export function parsePluginsTxt(raw, pluginsTxtPath = '') {
  const normalizedRaw = String(raw || '').replace(/^\uFEFF/, '');
  const lineEnding = normalizedRaw.includes('\r\n') ? '\r\n' : '\n';
  const sourceLines = normalizedRaw.length > 0 ? normalizedRaw.split(/\r?\n/) : [];

  return finalizePluginsTxtDocument({
    path: pluginsTxtPath,
    exists: true,
    readable: true,
    lineEnding: sourceLines.length > 0 ? lineEnding : '\r\n',
    lines: sourceLines.map((line) => parsePluginsTxtLine(line)),
  });
}

export function readPluginsTxt(pluginsTxtPath) {
  if (!pluginsTxtPath) {
    return finalizePluginsTxtDocument({
      path: '',
      exists: false,
      readable: false,
      lineEnding: '\r\n',
      lines: [],
      readError: 'Plugins.txt path is not configured.',
    });
  }

  if (!fs.existsSync(pluginsTxtPath)) {
    return createEmptyPluginsTxtDocument(pluginsTxtPath);
  }

  try {
    return parsePluginsTxt(fs.readFileSync(pluginsTxtPath, 'utf8'), pluginsTxtPath);
  } catch (error) {
    return finalizePluginsTxtDocument({
      path: pluginsTxtPath,
      exists: true,
      readable: false,
      lineEnding: '\r\n',
      lines: [],
      readError: String(error?.message || error),
    });
  }
}

export function serializePluginsTxt(document) {
  return document.lines
    .map((line) => {
      if (line.type === 'blank') {
        return '';
      }
      if (line.type === 'plugin') {
        return `${line.enabled ? '*' : ''}${line.name}`;
      }
      return line.raw ?? '';
    })
    .join(document.lineEnding || '\r\n');
}

function uniquePluginNames(pluginNames) {
  const seen = new Set();
  const uniqueNames = [];

  for (const pluginName of pluginNames) {
    const normalizedName = String(pluginName || '').trim();
    if (!isPluginFilename(normalizedName)) {
      continue;
    }

    const key = pluginKey(normalizedName);
    if (seen.has(key)) {
      continue;
    }

    seen.add(key);
    uniqueNames.push(normalizedName);
  }

  return uniqueNames;
}

export function setPluginEnabled(document, pluginName, enabled) {
  const normalizedName = String(pluginName || '').trim();
  if (!isPluginFilename(normalizedName)) {
    return finalizePluginsTxtDocument(document);
  }

  let matched = false;
  for (const line of document.lines) {
    if (line.type !== 'plugin') {
      continue;
    }
    if (pluginKey(line.name) !== pluginKey(normalizedName)) {
      continue;
    }
    line.enabled = enabled === true;
    matched = true;
  }

  if (!matched && enabled === true) {
    if (document.lines.length > 0 && document.lines.at(-1)?.type !== 'blank') {
      document.lines.push(createBlankLine());
    }
    document.lines.push(createPluginLine(normalizedName, true));
  }

  return finalizePluginsTxtDocument(document);
}

export function setPluginOrder(document, orderedPluginNames) {
  const nextOrderedPluginNames = uniquePluginNames([
    ...(orderedPluginNames ?? []),
    ...(document.orderedPluginNames ?? []),
  ]);

  const firstPluginIndex = document.lines.findIndex((line) => line.type === 'plugin');
  const leadingLines = firstPluginIndex === -1 ? [...document.lines] : document.lines.slice(0, firstPluginIndex);
  const trailingNonPluginLines =
    firstPluginIndex === -1
      ? []
      : document.lines.slice(firstPluginIndex).filter((line) => line.type !== 'plugin');

  const pluginLines = nextOrderedPluginNames.map((pluginName) => {
    const pluginState = document.pluginStateByName.get(pluginKey(pluginName));
    return createPluginLine(pluginState?.name ?? pluginName, pluginState?.enabled === true);
  });

  return finalizePluginsTxtDocument({
    ...document,
    lines: [...leadingLines, ...pluginLines, ...trailingNonPluginLines],
  });
}

export function writePluginsTxt(document) {
  if (!document.path) {
    throw new Error('Plugins.txt path is not configured.');
  }

  fs.mkdirSync(path.dirname(document.path), { recursive: true });
  fs.writeFileSync(document.path, serializePluginsTxt(document), 'utf8');
}
