import * as path from "path";
import { System, SystemContext } from "../systems/system";
import {
  ServerAddon,
  ServerAddonApi,
  ServerAddonCapabilities,
  ServerAddonCustomPacketHandler,
  ServerAddonModule,
  SKYMP_SERVER_ADDON_API_VERSION,
} from "../../../skymp5-addons-api/serverAddonHost";

type SettingsObject = Record<string, unknown> | null;
type ServerAddonApiInternal = ServerAddonApi & {
  __dispose: () => Promise<void>;
};

const isRecord = (value: unknown): value is Record<string, unknown> => {
  return typeof value === "object" && value !== null && !Array.isArray(value);
};

const getServerAddonModulePaths = (settings: SettingsObject): string[] => {
  const addonModules = isRecord(settings?.addonModules)
    ? settings.addonModules
    : null;
  const serverModules = addonModules?.server;
  if (!Array.isArray(serverModules)) {
    return [];
  }

  return serverModules.flatMap((entry) => {
    if (typeof entry !== "string") {
      return [];
    }

    const normalizedPath = entry.trim();
    return normalizedPath.length > 0 ? [normalizedPath] : [];
  });
};

const getAddonConfig = (
  settings: SettingsObject,
  addonId: string,
): unknown => {
  const addons = isRecord(settings?.addons) ? settings.addons : null;
  return addons?.[addonId];
};

const toAbsoluteModulePath = (
  moduleBasePath: string,
  modulePath: string,
): string => {
  if (path.isAbsolute(modulePath)) {
    return modulePath;
  }
  return path.resolve(moduleBasePath, modulePath);
};

const isServerAddon = (value: unknown): value is ServerAddon => {
  return isRecord(value);
};

const callAsyncAddonHook = async (
  addonId: string,
  hookName: string,
  hook: (() => void | Promise<void>) | undefined,
  log: (...args: any[]) => void,
): Promise<void> => {
  if (!hook) {
    return;
  }

  try {
    await hook();
  } catch (error) {
    console.error(`[addon:${addonId}] ${hookName} failed`, error);
    log(`[addon:${addonId}] ${hookName} failed`, error);
  }
};

const callSyncAddonHook = (
  addonId: string,
  hookName: string,
  hook: (() => void) | undefined,
  log: (...args: any[]) => void,
): void => {
  if (!hook) {
    return;
  }

  try {
    hook();
  } catch (error) {
    console.error(`[addon:${addonId}] ${hookName} failed`, error);
    log(`[addon:${addonId}] ${hookName} failed`, error);
  }
};

const getAddonId = (
  modulePath: string,
  addonModule: Partial<ServerAddonModule>,
): string => {
  if (typeof addonModule.addonId === "string" && addonModule.addonId.trim()) {
    return addonModule.addonId.trim();
  }

  return path.basename(modulePath, path.extname(modulePath));
};

const getServerAddonCapabilities = (ctx: SystemContext): ServerAddonCapabilities => {
  return {
    actorAngleZ: typeof (ctx.svr as Partial<{
      getActorAngleZ: (actorId: number) => number;
    }>).getActorAngleZ === "function",
    customPacketSubscriptions: true,
    dispose: true,
  };
};

const createServerAddonApi = (
  addonId: string,
  settings: SettingsObject,
  ctx: SystemContext,
  log: (...args: any[]) => void,
  packetSubscriptionsByAddonId: Map<string, Map<string, Set<ServerAddonCustomPacketHandler>>>,
): ServerAddonApiInternal => {
  const cleanupCallbacks = new Set<() => void>();
  const capabilities = getServerAddonCapabilities(ctx);

  const registerCleanup = (cleanup: () => void): (() => void) => {
    cleanupCallbacks.add(cleanup);
    return () => {
      if (!cleanupCallbacks.delete(cleanup)) {
        return;
      }

      cleanup();
    };
  };

  const dispose = async (): Promise<void> => {
    const pendingCleanups = Array.from(cleanupCallbacks);
    cleanupCallbacks.clear();

    for (const cleanup of pendingCleanups) {
      try {
        cleanup();
      } catch (error) {
        console.error(`[addon:${addonId}] cleanup failed`, error);
        log(`[addon:${addonId}] cleanup failed`, error);
      }
    }
  };

  return {
    capabilities,
    error: (...args: unknown[]) => log(`[addon:${addonId}]`, ...args),
    getActorAngleZ: (actorId: number): number | null => {
      const actorAngleGetter = (ctx.svr as Partial<{
        getActorAngleZ: (id: number) => number;
      }>).getActorAngleZ;
      if (typeof actorAngleGetter !== "function") {
        return null;
      }

      try {
        const angleZ = actorAngleGetter.call(ctx.svr, actorId);
        return typeof angleZ === "number" && Number.isFinite(angleZ) ? angleZ : null;
      } catch (_error) {
        return null;
      }
    },
    getActorCellOrWorld: (actorId: number): number | null => {
      try {
        const worldOrCell = ctx.svr.getActorCellOrWorld(actorId);
        return typeof worldOrCell === "number" ? worldOrCell : null;
      } catch (_error) {
        return null;
      }
    },
    getActorPos: (actorId: number): number[] | null => {
      try {
        const position = ctx.svr.getActorPos(actorId);
        return Array.isArray(position) && position.length >= 3 ? position : null;
      } catch (_error) {
        return null;
      }
    },
    getConfig: <T = unknown>(): T | undefined => {
      return getAddonConfig(settings, addonId) as T | undefined;
    },
    getUserActor: (userId: number): number | null => {
      try {
        const actorId = ctx.svr.getUserActor(userId);
        return typeof actorId === "number" && actorId > 0 ? actorId : null;
      } catch (_error) {
        return null;
      }
    },
    isConnected: (userId: number): boolean => {
      try {
        return !!ctx.svr.isConnected(userId);
      } catch (_error) {
        return false;
      }
    },
    log: (...args: unknown[]) => log(`[addon:${addonId}]`, ...args),
    onCustomPacket: (type, handler) => {
      const normalizedType = `${type}`.trim();
      if (!normalizedType) {
        return () => undefined;
      }

      let addonSubscriptions = packetSubscriptionsByAddonId.get(addonId);
      if (!addonSubscriptions) {
        addonSubscriptions = new Map<string, Set<ServerAddonCustomPacketHandler>>();
        packetSubscriptionsByAddonId.set(addonId, addonSubscriptions);
      }

      let handlers = addonSubscriptions.get(normalizedType);
      if (!handlers) {
        handlers = new Set<ServerAddonCustomPacketHandler>();
        addonSubscriptions.set(normalizedType, handlers);
      }

      handlers.add(handler);
      return registerCleanup(() => {
        const currentAddonSubscriptions = packetSubscriptionsByAddonId.get(addonId);
        const currentHandlers = currentAddonSubscriptions?.get(normalizedType);
        if (!currentHandlers) {
          return;
        }

        currentHandlers.delete(handler);
        if (currentHandlers.size === 0) {
          currentAddonSubscriptions?.delete(normalizedType);
        }
        if (currentAddonSubscriptions && currentAddonSubscriptions.size === 0) {
          packetSubscriptionsByAddonId.delete(addonId);
        }
      });
    },
    onSpawnAllowed: (handler) => {
      const wrappedHandler = (userId: number, profileId: number) => {
        handler(userId, profileId);
      };

      ctx.gm.on("spawnAllowed", wrappedHandler);
      return registerCleanup(() => {
        ctx.gm.off("spawnAllowed", wrappedHandler);
      });
    },
    addonId,
    sendCustomPacket: (
      userId: number,
      type: string,
      payload: Record<string, unknown>,
    ) => {
      const normalizedType = `${type}`.trim();
      if (!normalizedType) {
        log(`[addon:${addonId}] rejected sendCustomPacket with empty type`);
        return;
      }

      ctx.svr.sendCustomPacket(userId, JSON.stringify({
        ...payload,
        customPacketType: normalizedType,
      }));
    },
    version: SKYMP_SERVER_ADDON_API_VERSION,
    __dispose: dispose,
  };
};

const wrapAddon = (
  addonId: string,
  addon: ServerAddon,
  log: (...args: any[]) => void,
  api: ServerAddonApiInternal,
  packetSubscriptionsByAddonId: Map<string, Map<string, Set<ServerAddonCustomPacketHandler>>>,
): System => {
  return {
    connect: addon.connect
      ? (userId: number) => {
        callSyncAddonHook(addonId, "connect", () => addon.connect?.(userId), log);
      }
      : undefined,
    customPacket: addon.customPacket
      ? (userId, type, content) => {
        const addonSubscriptions = packetSubscriptionsByAddonId.get(addonId);
        const packetHandlers = addonSubscriptions?.get(type);

        packetHandlers?.forEach((handler) => {
          callSyncAddonHook(addonId, `onCustomPacket(${type})`, () => handler(userId, content), log);
        });
        callSyncAddonHook(addonId, "customPacket", () => addon.customPacket?.(userId, type, content), log);
      }
      : (userId, type, content) => {
        const addonSubscriptions = packetSubscriptionsByAddonId.get(addonId);
        const packetHandlers = addonSubscriptions?.get(type);
        packetHandlers?.forEach((handler) => {
          callSyncAddonHook(addonId, `onCustomPacket(${type})`, () => handler(userId, content), log);
        });
      },
    disconnect: addon.disconnect
      ? (userId: number) => {
        callSyncAddonHook(addonId, "disconnect", () => addon.disconnect?.(userId), log);
      }
      : undefined,
    disposeAsync: async () => {
      await callAsyncAddonHook(addonId, "dispose", () => addon.dispose?.(), log);
      await api.__dispose();
      packetSubscriptionsByAddonId.delete(addonId);
    },
    initAsync: addon.init
      ? async () => {
        await callAsyncAddonHook(addonId, "init", () => addon.init?.(), log);
      }
      : undefined,
    systemName: addon.systemName || `Addon(${addonId})`,
    updateAsync: addon.update
      ? async () => {
        await callAsyncAddonHook(addonId, "update", () => addon.update?.(), log);
      }
      : undefined,
  };
};

const disposeLoadedSystems = async (
  systems: System[],
  ctx: SystemContext,
  log: (...args: any[]) => void,
): Promise<void> => {
  for (const system of [...systems].reverse()) {
    if (!system.disposeAsync) {
      continue;
    }

    try {
      await system.disposeAsync(ctx);
    } catch (error) {
      console.error(`[addon-loader] failed to dispose "${system.systemName}"`, error);
      log(`[addon-loader] failed to dispose "${system.systemName}"`, error);
    }
  }
};

const disposePartialAddonApi = async (
  configuredModulePath: string,
  addonId: string,
  api: ServerAddonApiInternal,
  log: (...args: any[]) => void,
): Promise<void> => {
  try {
    await api.__dispose();
  } catch (error) {
    console.error(
      `[addon-loader] failed to cleanup partially initialized addon "${addonId}" from "${configuredModulePath}"`,
      error,
    );
    log(
      `[addon-loader] failed to cleanup partially initialized addon "${addonId}" from "${configuredModulePath}"`,
      error,
    );
  }
};

export const loadServerAddonSystems = async ({
  ctx,
  log,
  moduleBasePath,
  settings,
}: {
  ctx: SystemContext;
  log: (...args: any[]) => void;
  moduleBasePath: string;
  settings: SettingsObject;
}): Promise<System[]> => {
  const systems: System[] = [];
  const failedModulePaths: string[] = [];
  const loadedAddonIds = new Set<string>();
  const packetSubscriptionsByAddonId =
    new Map<string, Map<string, Set<ServerAddonCustomPacketHandler>>>();

  for (const configuredModulePath of getServerAddonModulePaths(settings)) {
    const resolvedModulePath = toAbsoluteModulePath(moduleBasePath, configuredModulePath);
    let api: ServerAddonApiInternal | undefined;
    let addonId = path.basename(configuredModulePath, path.extname(configuredModulePath));

    try {
      // eslint-disable-next-line @typescript-eslint/no-var-requires
      const addonModule = require(resolvedModulePath) as Partial<ServerAddonModule>;
      if (!isRecord(addonModule)) {
        throw new Error("Expected module to export an object");
      }
      if (typeof addonModule.createServerAddon !== "function") {
        throw new Error("Expected module to export createServerAddon(api, config)");
      }

      addonId = getAddonId(configuredModulePath, addonModule);
      if (loadedAddonIds.has(addonId)) {
        throw new Error(`Duplicate server addonId '${addonId}' in module '${configuredModulePath}'`);
      }

      api = createServerAddonApi(
        addonId,
        settings,
        ctx,
        log,
        packetSubscriptionsByAddonId,
      );
      const addon = addonModule.createServerAddon(api, api.getConfig());
      if (!isServerAddon(addon)) {
        throw new Error("Expected createServerAddon(api, config) to return an addon object");
      }

      log(`[addon:${addonId}] loaded from "${resolvedModulePath}"`);
      systems.push(wrapAddon(addonId, addon, log, api, packetSubscriptionsByAddonId));
      loadedAddonIds.add(addonId);
      api = undefined;
    } catch (error) {
      if (api) {
        await disposePartialAddonApi(configuredModulePath, addonId, api, log);
      }
      console.error(`[addon-loader] failed to load "${configuredModulePath}"`, error);
      log(`[addon-loader] failed to load "${configuredModulePath}"`, error);
      failedModulePaths.push(configuredModulePath);
    }
  }

  if (failedModulePaths.length > 0) {
    await disposeLoadedSystems(systems, ctx, log);
    const noun = failedModulePaths.length === 1 ? "module" : "modules";
    throw new Error(
      `Server addon startup aborted: ${failedModulePaths.length} configured addon ${noun} failed to load. See addon-loader logs for details.`,
    );
  }

  return systems;
};
