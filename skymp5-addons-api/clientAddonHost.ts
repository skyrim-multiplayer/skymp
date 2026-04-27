export const SKYMP_CLIENT_ADDON_HOST_NAME = "__skympClientAddonHost" as const;
export const SKYMP_CLIENT_ADDON_HOST_VERSION = 5 as const;

export type ClientAddonCapabilities = Record<string, never>;

export type ClientAddonRemoteAuthGameData = {
  session: string;
  masterApiId: number;
  discordUsername: string | null;
  discordDiscriminator: string | null;
  discordAvatar: string | null;
};

export type ClientAddonLocalAuthGameData = {
  profileId: number;
};

export type ClientAddonAuthGameData = {
  remote?: ClientAddonRemoteAuthGameData;
  local?: ClientAddonLocalAuthGameData;
};

export type ClientAddonConnectionDeniedEvent = {
  error: string;
};

export type ClientAddonBrowserApi = {
  emitEvent: (eventName: string, dataJson: string) => void;
  getBackendName: () => string;
  isFocused: () => boolean;
  isVisible: () => boolean;
  loadUrl: (url: string) => void;
  setFocused: (focused: boolean) => void;
  setVisible: (visible: boolean) => void;
};

export type ClientAddonInputState = {
  isDown: (binding: number[]) => boolean;
};

export type ClientAddonAuthNeededHandler = () => void;
export type ClientAddonConnectionAcceptedHandler = () => void;
export type ClientAddonConnectionDeniedHandler = (
  event: ClientAddonConnectionDeniedEvent,
) => void;
export type ClientAddonConnectionDisconnectHandler = () => void;
export type ClientAddonConnectionFailedHandler = () => void;

export type ClientAddonAuthApi = {
  onAuthNeeded: (
    handler: ClientAddonAuthNeededHandler,
  ) => ClientAddonUnsubscribe;
  onConnectionAccepted: (
    handler: ClientAddonConnectionAcceptedHandler,
  ) => ClientAddonUnsubscribe;
  onConnectionDenied: (
    handler: ClientAddonConnectionDeniedHandler,
  ) => ClientAddonUnsubscribe;
  onConnectionDisconnect: (
    handler: ClientAddonConnectionDisconnectHandler,
  ) => ClientAddonUnsubscribe;
  onConnectionFailed: (
    handler: ClientAddonConnectionFailedHandler,
  ) => ClientAddonUnsubscribe;
  submitAuthAttempt: (authGameData: ClientAddonAuthGameData) => void;
};

export type ClientAddonLocalSpawnEvent = {
  profileId: number | null;
};

export type ClientAddonBrowserMessageHandler = (args: unknown[]) => void;
export type ClientAddonCustomPacketHandler = (payload: Record<string, unknown>) => void;
export type ClientAddonInputHandler = (inputState: ClientAddonInputState) => void;
export type ClientAddonLocalSpawnHandler = (event: ClientAddonLocalSpawnEvent) => void;
export type ClientAddonTickHandler = () => void;
export type ClientAddonUnsubscribe = () => void;

export type ClientAddonApi = {
  auth: ClientAddonAuthApi;
  browser: ClientAddonBrowserApi;
  capabilities: ClientAddonCapabilities;
  getLocalProfileId: () => number | null;
  getSettingsScope: <T = Record<string, unknown>>(scope: string) => T | undefined;
  log: (...args: unknown[]) => void;
  logError: (...args: unknown[]) => void;
  onBrowserMessage: (
    key: string,
    handler: ClientAddonBrowserMessageHandler,
  ) => ClientAddonUnsubscribe;
  onCustomPacket: (
    type: string,
    handler: ClientAddonCustomPacketHandler,
  ) => ClientAddonUnsubscribe;
  onInputState: (handler: ClientAddonInputHandler) => ClientAddonUnsubscribe;
  onLocalSpawn: (
    handler: ClientAddonLocalSpawnHandler,
  ) => ClientAddonUnsubscribe;
  onTick: (handler: ClientAddonTickHandler) => ClientAddonUnsubscribe;
  resolveScanCode: (keyName: string) => number | null;
  sendCustomPacket: (
    type: string,
    payload: Record<string, unknown>,
    reliability?: "reliable" | "unreliable",
  ) => void;
  version: number;
};

export type ClientAddonInit = (api: ClientAddonApi) => void | Promise<void>;

export type ClientAddonRuntimeRegistrar = (
  addonId: string,
  init: ClientAddonInit,
) => void;

export type QueuedClientAddonRegistration = {
  init: ClientAddonInit;
  addonId: string;
};

export type ClientAddonHostGlobal = {
  registerClientAddon: (addonId: string, init: ClientAddonInit) => void;
  version: number | null;
};

type ClientAddonHostGlobalInternal = ClientAddonHostGlobal & {
  __pendingRegistrations: QueuedClientAddonRegistration[];
  __runtimeRegistrar: ClientAddonRuntimeRegistrar | null;
};

declare global {
  // eslint-disable-next-line no-var
  var __skympClientAddonHost: ClientAddonHostGlobalInternal | undefined;
}

export const ensureClientAddonHostGlobal = (): ClientAddonHostGlobal => {
  if (!globalThis.__skympClientAddonHost) {
    globalThis.__skympClientAddonHost = {
      __pendingRegistrations: [],
      __runtimeRegistrar: null,
      registerClientAddon: (addonId, init) => {
        const host = ensureClientAddonHostGlobalInternal();
        if (host.__runtimeRegistrar) {
          host.__runtimeRegistrar(addonId, init);
          return;
        }

        host.__pendingRegistrations.push({ init, addonId });
      },
      version: null,
    };
  }

  return globalThis.__skympClientAddonHost;
};

export const activateClientAddonHostGlobal = (
  runtimeRegistrar: ClientAddonRuntimeRegistrar,
): ClientAddonHostGlobal => {
  const host = ensureClientAddonHostGlobalInternal();
  host.__runtimeRegistrar = runtimeRegistrar;
  host.version = SKYMP_CLIENT_ADDON_HOST_VERSION;

  const pending = host.__pendingRegistrations.splice(0);
  for (const registration of pending) {
    runtimeRegistrar(registration.addonId, registration.init);
  }

  return host;
};

const ensureClientAddonHostGlobalInternal = (): ClientAddonHostGlobalInternal => {
  return ensureClientAddonHostGlobal() as ClientAddonHostGlobalInternal;
};
