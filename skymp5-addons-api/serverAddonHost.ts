export type ServerAddonContent = Record<string, any>;
export type ServerAddonCustomPacketHandler = (
  userId: number,
  content: ServerAddonContent,
) => void;
export type ServerAddonCapabilities = {
  actorAngleZ: boolean;
  customPacketSubscriptions: boolean;
  dispose: boolean;
};

export type ServerAddonApi = {
  capabilities: ServerAddonCapabilities;
  error: (...args: unknown[]) => void;
  getActorAngleZ: (actorId: number) => number | null;
  getActorCellOrWorld: (actorId: number) => number | null;
  getActorPos: (actorId: number) => number[] | null;
  getConfig: <T = unknown>() => T | undefined;
  getUserActor: (userId: number) => number | null;
  isConnected: (userId: number) => boolean;
  log: (...args: unknown[]) => void;
  onCustomPacket: (
    type: string,
    handler: ServerAddonCustomPacketHandler,
  ) => (() => void);
  onSpawnAllowed: (
    handler: (userId: number, profileId: number) => void,
  ) => (() => void);
  addonId: string;
  sendCustomPacket: (
    userId: number,
    type: string,
    payload: Record<string, unknown>,
  ) => void;
  version: number;
};

export type ServerAddon = {
  connect?: (userId: number) => void;
  customPacket?: (
    userId: number,
    type: string,
    content: ServerAddonContent,
  ) => void;
  disconnect?: (userId: number) => void;
  dispose?: () => void | Promise<void>;
  init?: () => void | Promise<void>;
  systemName?: string;
  update?: () => void | Promise<void>;
};

export type ServerAddonModule = {
  createServerAddon: (api: ServerAddonApi, config: unknown) => ServerAddon;
  addonId?: string;
};

export const SKYMP_SERVER_ADDON_API_VERSION = 3 as const;
