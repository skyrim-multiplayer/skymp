import { ServerAddonModule } from "../../../../skymp5-addons-api/serverAddonHost";
import { pushServerAddonLoaderTestEvent } from "./serverAddonLoaderTestState";

export const addonId = "loader-fixture";

export const createServerAddon: ServerAddonModule["createServerAddon"] = (
  api,
) => {
  pushServerAddonLoaderTestEvent("duplicateCreateServerAddon", {
    addonId: api.addonId,
  });
  return {
    systemName: "DuplicateAddon",
  };
};
