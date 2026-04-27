import {
  ClientAddonApi,
  ClientAddonAuthGameData,
  ClientAddonAuthNeededHandler,
  ClientAddonBrowserApi,
  ClientAddonBrowserMessageHandler,
  ClientAddonCapabilities,
  ClientAddonConnectionAcceptedHandler,
  ClientAddonConnectionDeniedEvent,
  ClientAddonConnectionDeniedHandler,
  ClientAddonConnectionDisconnectHandler,
  ClientAddonConnectionFailedHandler,
  ClientAddonCustomPacketHandler,
  ClientAddonInit,
  ClientAddonInputHandler,
  ClientAddonInputState,
  ClientAddonLocalSpawnEvent,
  ClientAddonLocalSpawnHandler,
  ClientAddonTickHandler,
  ClientAddonUnsubscribe,
  SKYMP_CLIENT_ADDON_HOST_VERSION,
} from "../../../../skymp5-addons-api/clientAddonHost";

type ClientAddonRuntimeEnvironment = {
  auth: {
    submitAuthAttempt: (authGameData: ClientAddonAuthGameData) => void;
  };
  browser: ClientAddonBrowserApi;
  capabilities: ClientAddonCapabilities;
  getLocalProfileId: () => number | null;
  getSettingsScope: <T = Record<string, unknown>>(scope: string) => T | undefined;
  log: (addonId: string, ...args: unknown[]) => void;
  logError: (addonId: string, ...args: unknown[]) => void;
  resolveScanCode: (keyName: string) => number | null;
  sendCustomPacket: (
    type: string,
    payload: Record<string, unknown>,
    reliability: "reliable" | "unreliable",
  ) => void;
};

export class ClientAddonHostRuntime {
  public constructor(private environment: ClientAddonRuntimeEnvironment) {}

  public registerAddon(addonId: string, init: ClientAddonInit): void {
    const normalizedAddonId = addonId.trim();
    if (!normalizedAddonId) {
      this.environment.logError("ClientAddonHost", "Rejected client addon registration with empty addonId");
      return;
    }

    if (this.addonApis.has(normalizedAddonId)) {
      this.environment.logError(
        "ClientAddonHost",
        `Rejected duplicate client addon registration for '${normalizedAddonId}'`,
      );
      return;
    }

    const api = this.createAddonApi(normalizedAddonId);
    this.addonApis.set(normalizedAddonId, api);
    this.environment.log("ClientAddonHost", "Registered client addon", {
      addonId: normalizedAddonId,
    });

    Promise.resolve(init(api)).catch((error) => {
      this.environment.logError(
        "ClientAddonHost",
        `Client addon '${normalizedAddonId}' initialization failed`,
        error,
      );
    });
  }

  public dispatchAuthNeeded(): void {
    this.invokeSetHandlers(
      this.authNeededHandlers,
      (handler) => {
        handler();
      },
      "auth-needed handler failed",
    );
  }

  public dispatchBrowserMessage(key: string, args: unknown[]): void {
    this.invokeMapHandlers(
      this.browserMessageHandlers,
      key,
      (handler) => {
        handler(args);
      },
      `browser handler failed for '${key}'`,
    );
  }

  public dispatchConnectionAccepted(): void {
    this.invokeSetHandlers(
      this.connectionAcceptedHandlers,
      (handler) => {
        handler();
      },
      "connection-accepted handler failed",
    );
  }

  public dispatchConnectionDenied(event: ClientAddonConnectionDeniedEvent): void {
    this.invokeSetHandlers(
      this.connectionDeniedHandlers,
      (handler) => {
        handler(event);
      },
      "connection-denied handler failed",
    );
  }

  public dispatchConnectionDisconnect(): void {
    this.invokeSetHandlers(
      this.connectionDisconnectHandlers,
      (handler) => {
        handler();
      },
      "connection-disconnect handler failed",
    );
  }

  public dispatchConnectionFailed(): void {
    this.invokeSetHandlers(
      this.connectionFailedHandlers,
      (handler) => {
        handler();
      },
      "connection-failed handler failed",
    );
  }

  public dispatchCustomPacket(type: string, payload: Record<string, unknown>): void {
    this.invokeMapHandlers(
      this.customPacketHandlers,
      type,
      (handler) => {
        handler(payload);
      },
      `custom packet handler failed for '${type}'`,
    );
  }

  public dispatchInputState(inputState: ClientAddonInputState): void {
    this.invokeSetHandlers(
      this.inputHandlers,
      (handler) => {
        handler(inputState);
      },
      "input handler failed",
    );
  }

  public dispatchLocalSpawn(payload: ClientAddonLocalSpawnEvent): void {
    this.hasLocalSpawned = true;
    this.lastLocalSpawnEvent = payload;
    this.invokeSetHandlers(
      this.localSpawnHandlers,
      (handler) => {
        handler(payload);
      },
      "local spawn handler failed",
    );
  }

  public dispatchTick(): void {
    this.invokeSetHandlers(
      this.tickHandlers,
      (handler) => {
        handler();
      },
      "tick handler failed",
    );
  }

  private createAddonApi(addonId: string): ClientAddonApi {
    return {
      auth: {
        onAuthNeeded: (handler: ClientAddonAuthNeededHandler): ClientAddonUnsubscribe => {
          return this.subscribeSetValue(this.authNeededHandlers, handler);
        },
        onConnectionAccepted: (
          handler: ClientAddonConnectionAcceptedHandler,
        ): ClientAddonUnsubscribe => {
          return this.subscribeSetValue(this.connectionAcceptedHandlers, handler);
        },
        onConnectionDenied: (
          handler: ClientAddonConnectionDeniedHandler,
        ): ClientAddonUnsubscribe => {
          return this.subscribeSetValue(this.connectionDeniedHandlers, handler);
        },
        onConnectionDisconnect: (
          handler: ClientAddonConnectionDisconnectHandler,
        ): ClientAddonUnsubscribe => {
          return this.subscribeSetValue(this.connectionDisconnectHandlers, handler);
        },
        onConnectionFailed: (
          handler: ClientAddonConnectionFailedHandler,
        ): ClientAddonUnsubscribe => {
          return this.subscribeSetValue(this.connectionFailedHandlers, handler);
        },
        submitAuthAttempt: (authGameData: ClientAddonAuthGameData) => {
          this.environment.auth.submitAuthAttempt(authGameData);
        },
      },
      browser: this.environment.browser,
      capabilities: this.environment.capabilities,
      getLocalProfileId: () => this.environment.getLocalProfileId(),
      getSettingsScope: <T = Record<string, unknown>>(scope: string) => {
        return this.environment.getSettingsScope<T>(scope);
      },
      log: (...args: unknown[]) => this.environment.log(`ClientAddon(${addonId})`, ...args),
      logError: (...args: unknown[]) => this.environment.logError(`ClientAddon(${addonId})`, ...args),
      onBrowserMessage: (
        key: string,
        handler: ClientAddonBrowserMessageHandler,
      ): ClientAddonUnsubscribe => {
        return this.subscribeMapValue(this.browserMessageHandlers, key, handler);
      },
      onCustomPacket: (
        type: string,
        handler: ClientAddonCustomPacketHandler,
      ): ClientAddonUnsubscribe => {
        return this.subscribeMapValue(this.customPacketHandlers, type, handler);
      },
      onInputState: (handler: ClientAddonInputHandler): ClientAddonUnsubscribe => {
        return this.subscribeSetValue(this.inputHandlers, handler);
      },
      onLocalSpawn: (
        handler: ClientAddonLocalSpawnHandler,
      ): ClientAddonUnsubscribe => {
        const unsubscribe = this.subscribeSetValue(this.localSpawnHandlers, handler);
        if (this.hasLocalSpawned && this.lastLocalSpawnEvent) {
          this.invokeAddonHandler(
            addonId,
            "local spawn handler failed",
            () => {
              handler(this.lastLocalSpawnEvent as ClientAddonLocalSpawnEvent);
            },
          );
        }

        return unsubscribe;
      },
      onTick: (handler: ClientAddonTickHandler): ClientAddonUnsubscribe => {
        return this.subscribeSetValue(this.tickHandlers, handler);
      },
      resolveScanCode: (keyName: string): number | null => {
        return this.environment.resolveScanCode(keyName);
      },
      sendCustomPacket: (
        type: string,
        payload: Record<string, unknown>,
        reliability: "reliable" | "unreliable" = "reliable",
      ) => {
        this.environment.sendCustomPacket(type, payload, reliability);
      },
      version: SKYMP_CLIENT_ADDON_HOST_VERSION,
    };
  }

  private invokeMapHandlers<THandler>(
    target: Map<string, Set<THandler>>,
    key: string,
    invoke: (handler: THandler) => void,
    messageSuffix: string,
  ): void {
    const handlers = target.get(key);
    if (!handlers || handlers.size === 0) {
      return;
    }

    this.invokeSetHandlers(handlers, invoke, messageSuffix);
  }

  private invokeAddonHandler(
    addonId: string,
    messageSuffix: string,
    invoke: () => void,
  ): void {
    try {
      invoke();
    } catch (error) {
      this.environment.logError(`ClientAddon(${addonId})`, messageSuffix, error);
    }
  }

  private invokeSetHandlers<THandler>(
    handlers: Set<THandler>,
    invoke: (handler: THandler) => void,
    messageSuffix: string,
  ): void {
    handlers.forEach((handler) => {
      try {
        invoke(handler);
      } catch (error) {
        this.environment.logError("ClientAddonHost", messageSuffix, error);
      }
    });
  }

  private subscribeMapValue<THandler>(
    target: Map<string, Set<THandler>>,
    key: string,
    handler: THandler,
  ): ClientAddonUnsubscribe {
    let handlers = target.get(key);
    if (!handlers) {
      handlers = new Set<THandler>();
      target.set(key, handlers);
    }

    handlers.add(handler);
    return () => {
      const currentHandlers = target.get(key);
      if (!currentHandlers) {
        return;
      }

      currentHandlers.delete(handler);
      if (currentHandlers.size === 0) {
        target.delete(key);
      }
    };
  }

  private subscribeSetValue<THandler>(
    target: Set<THandler>,
    handler: THandler,
  ): ClientAddonUnsubscribe {
    target.add(handler);
    return () => {
      target.delete(handler);
    };
  }

  private readonly authNeededHandlers = new Set<ClientAddonAuthNeededHandler>();
  private readonly browserMessageHandlers =
    new Map<string, Set<ClientAddonBrowserMessageHandler>>();
  private readonly connectionAcceptedHandlers =
    new Set<ClientAddonConnectionAcceptedHandler>();
  private readonly connectionDeniedHandlers =
    new Set<ClientAddonConnectionDeniedHandler>();
  private readonly connectionDisconnectHandlers =
    new Set<ClientAddonConnectionDisconnectHandler>();
  private readonly connectionFailedHandlers =
    new Set<ClientAddonConnectionFailedHandler>();
  private readonly customPacketHandlers =
    new Map<string, Set<ClientAddonCustomPacketHandler>>();
  private hasLocalSpawned = false;
  private readonly inputHandlers = new Set<ClientAddonInputHandler>();
  private lastLocalSpawnEvent: ClientAddonLocalSpawnEvent | null = null;
  private readonly localSpawnHandlers = new Set<ClientAddonLocalSpawnHandler>();
  private readonly addonApis = new Map<string, ClientAddonApi>();
  private readonly tickHandlers = new Set<ClientAddonTickHandler>();
}
