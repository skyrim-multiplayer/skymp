import fs from 'fs';
import path from 'path';

const PLUGIN_EXTENSIONS = new Set(['.esp', '.esm', '.esl']);
const ARCHIVE_EXTENSIONS = new Set(['.bsa', '.ba2']);
const VANILLA_PLUGIN_NAMES = new Set([
  'skyrim.esm',
  'update.esm',
  'dawnguard.esm',
  'hearthfires.esm',
  'dragonborn.esm',
]);

function safeReadDirectory(dirPath) {
  try {
    return fs.readdirSync(dirPath, { withFileTypes: true });
  } catch {
    return [];
  }
}

function toRelativePath(...parts) {
  return parts.filter(Boolean).join('/').replace(/\\/g, '/');
}

function pluginKey(value) {
  return String(value || '').trim().toLowerCase();
}

function createInventoryEntry({
  id,
  name,
  kind,
  primaryPath,
  relatedPaths = [],
  totalFiles = 1,
  pluginName = null,
  toggleMethod = 'none',
  existsOnDisk = true,
  notes = [],
}) {
  return {
    id,
    name,
    kind,
    primaryPath,
    relatedPaths,
    totalFiles,
    pluginName,
    toggleMethod,
    existsOnDisk,
    notes,
  };
}

function isPluginFile(filename) {
  return PLUGIN_EXTENSIONS.has(path.extname(filename).toLowerCase());
}

function isArchiveFile(filename) {
  return ARCHIVE_EXTENSIONS.has(path.extname(filename).toLowerCase());
}

function shouldIgnoreVanillaPlugin(filename) {
  return VANILLA_PLUGIN_NAMES.has(pluginKey(filename));
}

function archiveMatchesPlugin(archiveFilename, pluginStem) {
  const archiveStem = path.basename(archiveFilename, path.extname(archiveFilename)).toLowerCase();
  const normalizedPluginStem = pluginStem.toLowerCase();
  return (
    archiveStem === normalizedPluginStem ||
    archiveStem.startsWith(`${normalizedPluginStem} `) ||
    archiveStem.startsWith(`${normalizedPluginStem}-`) ||
    archiveStem.startsWith(`${normalizedPluginStem}_`)
  );
}

function buildPluginBackedEntries(dataDirEntries) {
  const pluginFiles = dataDirEntries.filter(
    (entry) =>
      entry.isFile() && isPluginFile(entry.name) && !shouldIgnoreVanillaPlugin(entry.name),
  );
  const archiveFiles = dataDirEntries.filter((entry) => entry.isFile() && isArchiveFile(entry.name));

  return pluginFiles.map((pluginFile) => {
    const pluginStem = path.basename(pluginFile.name, path.extname(pluginFile.name));
    const relatedArchives = archiveFiles.filter((archiveFile) =>
      archiveMatchesPlugin(archiveFile.name, pluginStem),
    );
    const relatedPaths = [
      toRelativePath(pluginFile.name),
      ...relatedArchives.map((archiveFile) => toRelativePath(archiveFile.name)),
    ];

    return createInventoryEntry({
      id: `plugin:${pluginKey(pluginFile.name)}`,
      name: pluginStem,
      kind: 'plugin_backed',
      primaryPath: toRelativePath(pluginFile.name),
      relatedPaths,
      totalFiles: relatedPaths.length,
      pluginName: pluginFile.name,
      toggleMethod: 'plugins_txt',
      notes:
        relatedArchives.length > 0
          ? [`Includes ${relatedArchives.length} related archive file(s).`]
          : [],
    });
  });
}

function buildSksePluginEntries(ctx) {
  return safeReadDirectory(ctx.sksePluginsDir)
    .filter((entry) => entry.isFile() && path.extname(entry.name).toLowerCase() === '.dll')
    .map((entry) =>
      createInventoryEntry({
        id: `skse:${pluginKey(entry.name)}`,
        name: path.basename(entry.name, path.extname(entry.name)),
        kind: 'skse_plugin',
        primaryPath: toRelativePath('SKSE', 'Plugins', entry.name),
        relatedPaths: [toRelativePath('SKSE', 'Plugins', entry.name)],
      }),
    );
}

function buildPlatformPluginEntries(pluginsDir, kind, relativeRoot) {
  return safeReadDirectory(pluginsDir)
    .filter((entry) => entry.isFile() && path.extname(entry.name).toLowerCase() === '.js')
    .map((entry) =>
      createInventoryEntry({
        id: `${kind}:${pluginKey(entry.name)}`,
        name: path.basename(entry.name, path.extname(entry.name)),
        kind,
        primaryPath: toRelativePath(relativeRoot, entry.name),
        relatedPaths: [toRelativePath(relativeRoot, entry.name)],
      }),
    );
}

function buildTopLevelFolderEntries(dataDir, dataDirEntries) {
  return dataDirEntries
    .filter((entry) => entry.isDirectory())
    .filter((entry) => !['skse', 'platform'].includes(entry.name.toLowerCase()))
    .map((entry) => {
      const childItemCount = safeReadDirectory(path.join(dataDir, entry.name)).length;

      return createInventoryEntry({
        id: `folder:${pluginKey(entry.name)}`,
        name: entry.name,
        kind: 'folder_group',
        primaryPath: toRelativePath(entry.name),
        relatedPaths: [toRelativePath(entry.name)],
        totalFiles: childItemCount,
        notes: childItemCount > 0 ? [`Contains ${childItemCount} top-level item(s).`] : [],
      });
    });
}

export function scanModsInventory(ctx) {
  const dataDir = ctx?.skyrimDataDir ? String(ctx.skyrimDataDir) : '';
  const dataDirExists = Boolean(dataDir) && fs.existsSync(dataDir);

  if (!dataDirExists) {
    return {
      checkedAt: new Date().toISOString(),
      dataDir,
      dataDirExists,
      entries: [],
    };
  }

  const dataDirEntries = safeReadDirectory(dataDir);
  const pluginBackedEntries = buildPluginBackedEntries(dataDirEntries);
  const sksePluginEntries = buildSksePluginEntries(ctx);
  const platformPluginEntries = buildPlatformPluginEntries(
    ctx.skyrimPlatformPluginDir,
    'platform_plugin',
    toRelativePath('Platform', 'Plugins'),
  );
  const platformDevPluginEntries = buildPlatformPluginEntries(
    ctx.skyrimPlatformPluginsDevDir,
    'platform_dev_plugin',
    toRelativePath('Platform', 'PluginsDev'),
  );
  const topLevelFolderEntries = buildTopLevelFolderEntries(dataDir, dataDirEntries);

  const entries = [
    ...pluginBackedEntries,
    ...platformPluginEntries,
    ...platformDevPluginEntries,
    ...sksePluginEntries,
    ...topLevelFolderEntries,
  ].sort((left, right) => {
    const kindOrder = [
      'plugin_backed',
      'platform_plugin',
      'platform_dev_plugin',
      'skse_plugin',
      'folder_group',
    ];
    const leftKindIndex = kindOrder.indexOf(left.kind);
    const rightKindIndex = kindOrder.indexOf(right.kind);
    if (leftKindIndex !== rightKindIndex) {
      return leftKindIndex - rightKindIndex;
    }
    return left.name.localeCompare(right.name);
  });

  return {
    checkedAt: new Date().toISOString(),
    dataDir,
    dataDirExists,
    entries,
  };
}
