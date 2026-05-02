import { ServerAddonModule } from "../../../../skymp5-addons-api/serverAddonHost";
import { pushServerAddonLoaderTestEvent } from "./serverAddonLoaderTestState";

export const addonId = "throwing-fixture";

export const createServerAddon: ServerAddonModule["createServerAddon"] = (
  api,
) => {
  pushServerAddonLoaderTestEvent("throwingCreateServerAddon", {
    addonId: api.addonId,
  });

  api.onSpawnAllowed((userId, profileId) => {
    pushServerAddonLoaderTestEvent("throwingSpawnAllowed", {
      profileId,
      userId,
    });
  });

  throw new Error("createServerAddon boom");
};
