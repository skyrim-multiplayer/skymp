import {
  ServerAddon,
  ServerAddonModule,
} from "../../../../skymp5-addons-api/serverAddonHost";
import { pushServerAddonLoaderTestEvent } from "./serverAddonLoaderTestState";

type LoaderFixtureConfig = {
  greeting?: string;
};

class SubscriptionsAddon implements ServerAddon {
  public systemName = "SubscriptionsAddon";

  public constructor(
    private greeting: string,
    private actorAngleSnapshot: number | null,
    private sendInitPacket: () => void,
  ) {}

  public async init(): Promise<void> {
    await Promise.resolve();
    this.sendInitPacket();
    pushServerAddonLoaderTestEvent("init", {
      actorAngleSnapshot: this.actorAngleSnapshot,
      greeting: this.greeting,
    });
  }

  public connect(userId: number): void {
    pushServerAddonLoaderTestEvent("connect", {
      userId,
    });
  }

  public customPacket(userId: number, type: string, content: Record<string, unknown>): void {
    pushServerAddonLoaderTestEvent("legacyCustomPacket", {
      content,
      type,
      userId,
    });
  }

  public disconnect(userId: number): void {
    pushServerAddonLoaderTestEvent("disconnect", {
      userId,
    });
  }

  public async dispose(): Promise<void> {
    await Promise.resolve();
    pushServerAddonLoaderTestEvent("dispose");
  }

  public async update(): Promise<void> {
    await Promise.resolve();
    pushServerAddonLoaderTestEvent("update");
  }
}

export const addonId = "loader-fixture";

export const createServerAddon: ServerAddonModule["createServerAddon"] = (
  api,
  config,
) => {
  pushServerAddonLoaderTestEvent("createServerAddon", {
    capabilities: api.capabilities,
    config,
    addonId: api.addonId,
  });

  api.onSpawnAllowed((userId, profileId) => {
    pushServerAddonLoaderTestEvent("spawnAllowed", {
      profileId,
      userId,
    });
  });
  api.onCustomPacket("loader:test", (userId, content) => {
    pushServerAddonLoaderTestEvent("typedCustomPacket", {
      content,
      userId,
    });
  });

  return new SubscriptionsAddon(
    ((config || {}) as LoaderFixtureConfig).greeting || "hi",
    api.getActorAngleZ(123),
    () => {
      api.sendCustomPacket(7, "loader:init", {
        greeting: ((config || {}) as LoaderFixtureConfig).greeting || "hi",
      });
    },
  );
};
