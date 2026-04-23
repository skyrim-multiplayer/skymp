import path from 'path';

import { scanModsInventory } from './mods-inventory.js';
import { readPluginsTxt, setPluginEnabled, setPluginOrder, writePluginsTxt } from './plugins-txt.js';

function createModsError(message, code, extra = {}) {
  const error = new Error(message);
  error.code = code;
  Object.assign(error, extra);
  return error;
}

function pluginKey(value) {
  return String(value || '').trim().toLowerCase();
}

function toIssue(code, message) {
  return { code, message };
}

function isPluginTableMod(mod) {
  return mod.kind === 'plugin_backed' || mod.kind === 'missing_plugin';
}

function summarizeMods(mods) {
  return mods.reduce(
    (summary, mod) => {
      summary.total += 1;
      summary.byKind[mod.kind] = (summary.byKind[mod.kind] ?? 0) + 1;

      if (mod.state === 'enabled') {
        summary.enabled += 1;
      } else if (mod.state === 'disabled') {
        summary.disabled += 1;
      } else {
        summary.inventoryOnly += 1;
      }

      if (mod.toggleable) {
        summary.toggleable += 1;
      }
      if (mod.attention) {
        summary.attention += 1;
      }

      return summary;
    },
    {
      total: 0,
      enabled: 0,
      disabled: 0,
      inventoryOnly: 0,
      attention: 0,
      toggleable: 0,
      byKind: {},
    },
  );
}

function getToggleStatus({ entry, pluginsDoc, runtimeStatus, pluginsTxtPath }) {
  if (runtimeStatus?.running) {
    return {
      toggleable: false,
      toggleReason: 'Stop Skyrim before changing plugin enable state.',
    };
  }

  if (entry.toggleMethod !== 'plugins_txt') {
    return {
      toggleable: false,
      toggleReason: 'This entry is inventory-only in the first version.',
    };
  }

  if (!pluginsTxtPath) {
    return {
      toggleable: false,
      toggleReason: 'Plugins.txt path is not configured.',
    };
  }

  if (pluginsDoc.readable === false) {
    return {
      toggleable: false,
      toggleReason: pluginsDoc.readError || 'Plugins.txt could not be read.',
    };
  }

  return {
    toggleable: true,
    toggleReason: '',
  };
}

function buildPluginBackedMod(entry, pluginsDoc, runtimeStatus, pluginsTxtPath) {
  const pluginState = pluginsDoc.pluginStateByName.get(pluginKey(entry.pluginName));
  const enabled = pluginState?.enabled === true;
  const issues = [];

  if (pluginState?.duplicate) {
    issues.push(
      toIssue(
        'plugins_txt_duplicate',
        `${entry.pluginName} appears multiple times in Plugins.txt.`,
      ),
    );
  }

  if (pluginState?.conflicting) {
    issues.push(
      toIssue(
        'plugins_txt_conflicting_entries',
        `${entry.pluginName} has conflicting enabled and disabled entries in Plugins.txt.`,
      ),
    );
  }

  const toggleStatus = getToggleStatus({
    entry,
    pluginsDoc,
    runtimeStatus,
    pluginsTxtPath,
  });

  return {
    ...entry,
    loadOrderIndex: pluginsDoc.pluginOrderByName.get(pluginKey(entry.pluginName)) ?? null,
    enabled,
    state: enabled ? 'enabled' : 'disabled',
    attention: issues.length > 0,
    issues,
    toggleable: toggleStatus.toggleable,
    toggleReason: toggleStatus.toggleReason,
  };
}

function buildInventoryOnlyMod(entry) {
  return {
    ...entry,
    loadOrderIndex: null,
    enabled: null,
    state: 'inventory_only',
    attention: false,
    issues: [],
    toggleable: false,
    toggleReason: 'This entry is inventory-only in the first version.',
  };
}

function buildMissingPluginEntries(pluginsDoc) {
  return Array.from(pluginsDoc.pluginStateByName.values()).map((pluginState) => ({
    id: `missing-plugin:${pluginKey(pluginState.name)}`,
    name: path.basename(pluginState.name, path.extname(pluginState.name)),
    kind: 'missing_plugin',
    primaryPath: pluginState.name,
    relatedPaths: [pluginState.name],
    totalFiles: 0,
    pluginName: pluginState.name,
    toggleMethod: 'none',
    existsOnDisk: false,
    loadOrderIndex: pluginsDoc.pluginOrderByName.get(pluginKey(pluginState.name)) ?? null,
    notes: [],
    enabled: pluginState.enabled === true,
    state: pluginState.enabled === true ? 'enabled' : 'disabled',
    attention: true,
    issues: [
      toIssue(
        'plugin_missing_on_disk',
        `${pluginState.name} is listed in Plugins.txt but is missing from the Skyrim Data directory.`,
      ),
    ],
    toggleable: false,
    toggleReason: 'The plugin file is missing from the Skyrim Data directory.',
  }));
}

function sortPluginTableMods(mods, pluginsDoc) {
  return [...mods].sort((left, right) => {
    const leftOrder = pluginsDoc.pluginOrderByName.get(pluginKey(left.pluginName));
    const rightOrder = pluginsDoc.pluginOrderByName.get(pluginKey(right.pluginName));

    if (leftOrder != null && rightOrder != null) {
      return leftOrder - rightOrder;
    }
    if (leftOrder != null) {
      return -1;
    }
    if (rightOrder != null) {
      return 1;
    }
    return left.name.localeCompare(right.name);
  });
}

export function getModsSnapshot(runtime, options = {}) {
  const runtimeStatus = options.runtimeStatus ?? null;
  const inventory = scanModsInventory(runtime?.ctx ?? {});
  const pluginsTxtPath = String(runtime?.ctx?.pluginsTxtPath || '');
  const pluginsDoc = readPluginsTxt(pluginsTxtPath);
  const pluginInventoryKeys = new Set(
    inventory.entries
      .filter((entry) => entry.kind === 'plugin_backed' && entry.pluginName)
      .map((entry) => pluginKey(entry.pluginName)),
  );

  const mods = inventory.entries.map((entry) => {
    if (entry.kind === 'plugin_backed') {
      return buildPluginBackedMod(entry, pluginsDoc, runtimeStatus, pluginsTxtPath);
    }
    return buildInventoryOnlyMod(entry);
  });

  const missingPluginEntries = buildMissingPluginEntries(pluginsDoc).filter(
    (entry) => !pluginInventoryKeys.has(pluginKey(entry.pluginName)),
  );

  const pluginTableMods = sortPluginTableMods(
    [
      ...missingPluginEntries,
      ...mods.filter((mod) => mod.kind === 'plugin_backed'),
    ],
    pluginsDoc,
  );
  const inventoryOnlyMods = mods.filter((mod) => mod.state === 'inventory_only');
  const mergedMods = [...pluginTableMods, ...inventoryOnlyMods];

  return {
    checkedAt: new Date().toISOString(),
    dataDir: {
      path: inventory.dataDir,
      exists: inventory.dataDirExists,
    },
    pluginsTxt: {
      path: pluginsTxtPath,
      exists: pluginsDoc.exists === true,
      readable: pluginsDoc.readable !== false,
      readError: pluginsDoc.readError ?? '',
      duplicatePluginNames: pluginsDoc.duplicatePluginNames ?? [],
      conflictingPluginNames: pluginsDoc.conflictingPluginNames ?? [],
      trackedPlugins: pluginsDoc.pluginStateByName.size,
    },
    runtime: {
      blocked: runtimeStatus?.running === true,
      reason:
        runtimeStatus?.running === true
          ? 'Stop Skyrim before changing plugin enable state.'
          : '',
    },
    mods: mergedMods,
    summary: summarizeMods(mergedMods),
  };
}

export function toggleMod(runtime, { modId, enabled }, options = {}) {
  const runtimeStatus = options.runtimeStatus ?? null;
  const snapshot = getModsSnapshot(runtime, { runtimeStatus });

  if (runtimeStatus?.running === true) {
    throw createModsError(
      'Stop Skyrim before changing plugin enable state.',
      'MOD_TOGGLE_BLOCKED_RUNTIME',
      { snapshot },
    );
  }

  const mod = snapshot.mods.find((entry) => entry.id === modId);
  if (!mod) {
    throw createModsError(`Unknown mod entry: ${modId}`, 'MOD_NOT_FOUND');
  }

  if (mod.toggleMethod !== 'plugins_txt' || mod.toggleable !== true) {
    throw createModsError(
      mod.toggleReason || `Mod ${mod.name} is not toggleable in this version.`,
      'MOD_NOT_TOGGLEABLE',
      { mod },
    );
  }

  if (!mod.pluginName) {
    throw createModsError(`Mod ${mod.name} has no plugin name to toggle.`, 'MOD_NOT_TOGGLEABLE', {
      mod,
    });
  }

  const pluginsDoc = readPluginsTxt(runtime?.ctx?.pluginsTxtPath || '');
  if (pluginsDoc.readable === false) {
    throw createModsError(
      pluginsDoc.readError || 'Plugins.txt could not be read.',
      'MOD_PLUGINS_TXT_UNREADABLE',
    );
  }

  setPluginEnabled(pluginsDoc, mod.pluginName, enabled === true);
  writePluginsTxt(pluginsDoc);

  const nextSnapshot = getModsSnapshot(runtime, { runtimeStatus });
  return {
    mod: nextSnapshot.mods.find((entry) => entry.id === mod.id) ?? null,
    snapshot: nextSnapshot,
  };
}

export function reorderMods(runtime, { orderedModIds }, options = {}) {
  const runtimeStatus = options.runtimeStatus ?? null;
  const snapshot = getModsSnapshot(runtime, { runtimeStatus });

  if (runtimeStatus?.running === true) {
    throw createModsError(
      'Stop Skyrim before changing plugin enable state.',
      'MOD_TOGGLE_BLOCKED_RUNTIME',
      { snapshot },
    );
  }

  if (!Array.isArray(orderedModIds) || orderedModIds.length === 0) {
    throw createModsError('orderedModIds must be a non-empty array.', 'MOD_REORDER_INVALID');
  }

  const uniqueOrderedModIds = Array.from(
    new Set(
      orderedModIds
        .map((modId) => String(modId || '').trim())
        .filter(Boolean),
    ),
  );
  if (uniqueOrderedModIds.length !== orderedModIds.length) {
    throw createModsError('orderedModIds must not contain duplicates.', 'MOD_REORDER_INVALID');
  }

  const pluginTableMods = snapshot.mods.filter(isPluginTableMod);
  const pluginTableModsById = new Map(pluginTableMods.map((mod) => [mod.id, mod]));
  for (const modId of uniqueOrderedModIds) {
    if (!pluginTableModsById.has(modId)) {
      throw createModsError(`Unknown plugin-backed mod entry: ${modId}`, 'MOD_REORDER_NOT_FOUND');
    }
  }

  const reorderedVisibleMods = uniqueOrderedModIds.map((modId) => pluginTableModsById.get(modId));
  const visibleModIds = new Set(uniqueOrderedModIds);
  let visibleIndex = 0;
  const reorderedPluginTableMods = pluginTableMods.map((mod) =>
    visibleModIds.has(mod.id) ? reorderedVisibleMods[visibleIndex++] : mod,
  );

  const pluginsDoc = readPluginsTxt(runtime?.ctx?.pluginsTxtPath || '');
  if (pluginsDoc.readable === false) {
    throw createModsError(
      pluginsDoc.readError || 'Plugins.txt could not be read.',
      'MOD_PLUGINS_TXT_UNREADABLE',
    );
  }

  const reorderedPluginsDoc = setPluginOrder(
    pluginsDoc,
    reorderedPluginTableMods.map((mod) => mod.pluginName).filter(Boolean),
  );
  writePluginsTxt(reorderedPluginsDoc);

  return {
    snapshot: getModsSnapshot(runtime, { runtimeStatus }),
  };
}
