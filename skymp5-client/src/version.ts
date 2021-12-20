import {
  Utility,
  Debug,
  getPlatformVersion,
  on,
  Game,
  Ui,
} from "skyrimPlatform";

const requiredVersion = "0.7.0+build3";

const realVersion =
  typeof getPlatformVersion === "function" ? getPlatformVersion() : "unknown";

export const verifyVersion = (): void => {
  if (!requiredVersion.includes(realVersion)) {
    Debug.messageBox(
      `You need to have on of those SkyrimPlatform versions ${JSON.stringify(
        requiredVersion
      )} to join this server. Your current version is ${realVersion}`
    );
    Utility.waitMenuMode(0.5).then(() => {
      on("update", () => {
        if (!Ui.isMenuOpen("MessageBoxMenu")) Game.quitToMainMenu();
      });
    });
  }
};
