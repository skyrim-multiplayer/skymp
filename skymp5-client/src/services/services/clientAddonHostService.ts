import { BrowserMessageEvent } from "skyrimPlatform";
import { AuthGameData, authGameDataStorageKey } from "../../features/authModel";
import { FunctionInfo } from "../../lib/functionInfo";
import { logError, logTrace } from "../../logging";
import { MsgType } from "../../messages";
import { AuthAttemptEvent } from "../events/authAttemptEvent";
import { ConnectionDenied } from "../events/connectionDenied";
import { ConnectionDisconnect } from "../events/connectionDisconnect";
import { ConnectionFailed } from "../events/connectionFailed";
import { QueryKeyCodeBindings } from "../events/queryKeyCodeBindings";
import { ConnectionMessage } from "../events/connectionMessage";
import { SendMessageEvent } from "../events/sendMessageEvent";
import { CreateActorMessage } from "../messages/createActorMessage";
import { CustomPacketMessage } from "../messages/customPacketMessage";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import {
  activateClientAddonHostGlobal,
  ClientAddonAuthGameData,
  ClientAddonBrowserApi,
  ClientAddonCapabilities,
  ClientAddonInputState,
  ClientAddonLocalSpawnEvent,
} from "../../../../skymp5-addons-api/clientAddonHost";
import { ClientAddonHostRuntime } from "./clientAddonHostRuntime";

declare const window: any;

type BrowserApiWithEmitEvent = Sp["browser"] & {
  emitEvent?: (eventName: string, dataJson: string) => void;
  getBackend?: () => {
    name?: string;
  };
};

const dispatchBrowserEventFallback = ({
  dataJson,
  eventName,
}: {
  dataJson: string;
  eventName: string;
}) => {
  const CustomEventCtor = window.CustomEvent;
  window.dispatchEvent(new CustomEventCtor("skymp-browser-event", {
    detail: {
      data: dataJson,
      eventName,
    },
  }));
};

export class ClientAddonHostService extends ClientListener {
  public constructor(private sp: Sp, private controller: CombinedController) {
    super();

    activateClientAddonHostGlobal((addonId, init) => {
      this.runtime.registerAddon(addonId, init);
    });

    this.controller.emitter.on("createActorMessage", (event) => {
      this.onCreateActorMessage(event);
    });
    this.controller.emitter.on("customPacketMessage", (event) => {
      this.onCustomPacketMessage(event);
    });
    this.controller.emitter.on("authNeeded", () => {
      this.runtime.dispatchAuthNeeded();
    });
    this.controller.emitter.on("connectionAccepted", () => {
      this.runtime.dispatchConnectionAccepted();
    });
    this.controller.emitter.on("connectionDenied", (event) => {
      this.onConnectionDenied(event);
    });
    this.controller.emitter.on("connectionDisconnect", (event) => {
      this.onConnectionDisconnect(event);
    });
    this.controller.emitter.on("connectionFailed", (event) => {
      this.onConnectionFailed(event);
    });
    this.controller.emitter.on("queryKeyCodeBindings", (event) => {
      this.onQueryKeyCodeBindings(event);
    });
    this.controller.on("browserMessage", (event) => this.onBrowserMessage(event));
    this.controller.on("tick", () => this.onTick());
  }

  private createBrowserApi(): ClientAddonBrowserApi {
    const browser: ClientAddonBrowserApi = {
      emitEvent: (eventName: string, dataJson: string) => {
        const browserApi = this.sp.browser as BrowserApiWithEmitEvent;
        if (typeof browserApi.emitEvent === "function") {
          browserApi.emitEvent(eventName, dataJson);
          return;
        }

        this.sp.browser.executeJavaScript(
          new FunctionInfo(dispatchBrowserEventFallback).getText({
            dataJson,
            eventName,
          }),
        );
      },
      getBackendName: () => this.getBrowserBackendName(),
      isFocused: () => this.sp.browser.isFocused(),
      isVisible: () => this.sp.browser.isVisible(),
      loadUrl: (url: string) => this.sp.browser.loadUrl(url),
      setFocused: (focused: boolean) => this.sp.browser.setFocused(focused),
      setVisible: (visible: boolean) => this.sp.browser.setVisible(visible),
    };

    return browser;
  }

  private onCreateActorMessage(event: ConnectionMessage<CreateActorMessage>): void {
    if (!event.message.isMe) {
      return;
    }

    this.runtime.dispatchLocalSpawn(this.createLocalSpawnEvent());
  }

  private onCustomPacketMessage(event: ConnectionMessage<CustomPacketMessage>): void {
    let payload: Record<string, unknown>;
    try {
      payload = JSON.parse(event.message.contentJsonDump);
    } catch (error) {
      logError(this, "Failed to parse custom packet JSON in addon host", error);
      return;
    }

    const type = typeof payload.customPacketType === "string"
      ? payload.customPacketType
      : "";
    if (!type) {
      return;
    }

    delete payload.customPacketType;

    this.runtime.dispatchCustomPacket(type, payload);
  }

  private onConnectionDenied(event: ConnectionDenied): void {
    this.runtime.dispatchConnectionDenied({
      error: event.error,
    });
  }

  private onConnectionDisconnect(_event: ConnectionDisconnect): void {
    this.runtime.dispatchConnectionDisconnect();
  }

  private onConnectionFailed(_event: ConnectionFailed): void {
    this.runtime.dispatchConnectionFailed();
  }

  private onQueryKeyCodeBindings(event: QueryKeyCodeBindings): void {
    const inputState: ClientAddonInputState = {
      isDown: (binding) => event.isDown(binding),
    };

    this.runtime.dispatchInputState(inputState);
  }

  private onBrowserMessage(event: BrowserMessageEvent): void {
    const eventArguments = Array.from(event.arguments || []);
    const key = typeof eventArguments[0] === "string"
      ? eventArguments[0]
      : "";
    if (!key) {
      return;
    }

    const args = eventArguments.slice(1);
    this.runtime.dispatchBrowserMessage(key, args);
  }

  private onTick(): void {
    this.runtime.dispatchTick();
  }

  private createLocalSpawnEvent(): ClientAddonLocalSpawnEvent {
    return {
      profileId: this.resolveLocalProfileId(),
    };
  }

  private resolveLocalProfileId(): number | null {
    const authGameData = this.sp.storage[authGameDataStorageKey] as AuthGameData | undefined;
    const authProfileId = authGameData?.local?.profileId as unknown;
    if (Number.isInteger(authProfileId)) {
      return authProfileId as number;
    }

    const settingsGameData = this.sp.settings["skymp5-client"]["gameData"] as
      | { profileId?: unknown }
      | undefined;
    const settingsProfileId = settingsGameData?.profileId as unknown;
    if (Number.isInteger(settingsProfileId)) {
      return settingsProfileId as number;
    }

    return null;
  }

  private getBrowserBackendName(): string {
    const browserApi = this.sp.browser as BrowserApiWithEmitEvent;
    if (typeof browserApi.getBackend !== "function") {
      return "unknown";
    }

    try {
      return String(browserApi.getBackend()?.name || "unknown");
    } catch (error) {
      logError(this, "Failed to read browser backend in addon host", error);
      return "error";
    }
  }

  private createCapabilities(): ClientAddonCapabilities {
    return {};
  }

  private readonly runtime = new ClientAddonHostRuntime({
    auth: {
      submitAuthAttempt: (authGameData: ClientAddonAuthGameData) => {
        const normalizedAuthGameData = this.normalizeAuthGameData(authGameData);
        if (!normalizedAuthGameData.local && !normalizedAuthGameData.remote) {
          logError(this, "Rejected client addon auth attempt with invalid auth data", authGameData);
          return;
        }

        const event: AuthAttemptEvent = {
          authGameData: normalizedAuthGameData,
        };
        this.controller.emitter.emit("authAttempt", event);
      },
    },
    browser: this.createBrowserApi(),
    capabilities: this.createCapabilities(),
    getLocalProfileId: () => this.resolveLocalProfileId(),
    getSettingsScope: <T = Record<string, unknown>>(scope: string) => {
      const settingsRoot = this.sp.settings as Record<string, unknown>;
      return settingsRoot[scope] as T | undefined;
    },
    log: (addonId: string, ...args: unknown[]) => logTrace(addonId, ...args),
    logError: (addonId: string, ...args: unknown[]) => logError(addonId, ...args),
    resolveScanCode: (keyName: string): number | null => {
      const runtimeMap = (this.sp as unknown as {
        DxScanCode?: Record<string, number>;
      }).DxScanCode;
      const scanCode = runtimeMap?.[keyName];
      return typeof scanCode === "number" ? scanCode : null;
    },
    sendCustomPacket: (
      type: string,
      payload: Record<string, unknown>,
      reliability: "reliable" | "unreliable",
    ) => {
      const event: SendMessageEvent<CustomPacketMessage> = {
        message: {
          t: MsgType.CustomPacket,
          contentJsonDump: JSON.stringify({
            ...payload,
            customPacketType: type,
          }),
        },
        reliability,
      };
      this.controller.emitter.emit("sendMessage", event);
    },
  });

  private normalizeAuthGameData(authGameData: ClientAddonAuthGameData): AuthGameData {
    const normalized: AuthGameData = {};

    const localProfileId = authGameData.local?.profileId;
    if (Number.isInteger(localProfileId)) {
      normalized.local = {
        profileId: localProfileId as number,
      };
    }

    const remote = authGameData.remote;
    if (
      remote &&
      typeof remote.session === "string" &&
      remote.session &&
      Number.isInteger(remote.masterApiId)
    ) {
      normalized.remote = {
        session: remote.session,
        masterApiId: remote.masterApiId,
        discordAvatar: this.normalizeNullableString(remote.discordAvatar),
        discordDiscriminator: this.normalizeNullableString(remote.discordDiscriminator),
        discordUsername: this.normalizeNullableString(remote.discordUsername),
      };
    }

    return normalized;
  }

  private normalizeNullableString(value: unknown): string | null {
    return typeof value === "string" ? value : null;
  }
}
