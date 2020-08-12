import {
  Utility,
  Debug,
  getPlatformVersion,
  on,
  Game,
  Ui,
} from "skyrimPlatform";

const requiredVersion = "0.7.0+build1";

const realVersion =
  typeof getPlatformVersion === "function" ? getPlatformVersion() : "unknown";

export const verifyVersion = (): void => {
  if (realVersion !== requiredVersion) {
    Debug.messageBox(
      `You need to have SkyrimPlatform ${requiredVersion} to join this server. Your current version is ${realVersion}`
    );
    Utility.waitMenuMode(0.5).then(() => {
      on("update", () => {
        if (!Ui.isMenuOpen("MessageBoxMenu")) Game.quitToMainMenu();
      });
    });
  }
};
