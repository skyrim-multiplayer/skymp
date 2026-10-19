import * as fs from 'fs';
import * as path from 'path';

export interface PluginStateEntry {
  pluginPath: string;
  fingerprint: string;
  version: string;
  startupEnabled: boolean;
  discoveredAt: string;
  updatedAt: string;
}

export interface PluginDiscoveryState {
  version: 1;
  plugins: Record<string, PluginStateEntry>;
}

export const PLUGINS_DIR = path.resolve(process.cwd(), 'Plugins');

export const PLUGIN_DISCOVERY_STATE_PATH = path.join(
  PLUGINS_DIR,
  '.discovery-state.json',
);

export const readDiscoveryState = (): PluginDiscoveryState => {
  if (!fs.existsSync(PLUGIN_DISCOVERY_STATE_PATH)) {
    return { version: 1, plugins: {} };
  }

  try {
    const stateRaw = fs.readFileSync(PLUGIN_DISCOVERY_STATE_PATH, 'utf8');
    const parsed = JSON.parse(stateRaw) as Partial<PluginDiscoveryState>;
    if (
      parsed.version !== 1 ||
      typeof parsed.plugins !== 'object' ||
      parsed.plugins === null
    ) {
      throw new Error('invalid discovery state format');
    }

    return {
      version: 1,
      plugins: parsed.plugins as Record<string, PluginStateEntry>,
    };
  } catch (e) {
    console.warn(
      `[plugins] Failed to parse "${PLUGIN_DISCOVERY_STATE_PATH}", starting with empty plugin state:`,
      e,
    );
    return { version: 1, plugins: {} };
  }
};

export const writeDiscoveryState = (state: PluginDiscoveryState): void => {
  fs.mkdirSync(path.dirname(PLUGIN_DISCOVERY_STATE_PATH), { recursive: true });
  const tmpPath = `${PLUGIN_DISCOVERY_STATE_PATH}.tmp`;
  fs.writeFileSync(tmpPath, JSON.stringify(state, null, 2), 'utf8');
  fs.renameSync(tmpPath, PLUGIN_DISCOVERY_STATE_PATH);
};
