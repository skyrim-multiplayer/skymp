import { settings, printConsole, findConsoleCommand } from "skyrimPlatform";
import { consoleCommands, scriptCommands } from "./consoleCommands";

export const blockConsole = (): void => {
  if (settings["skymp5-client"]["enable-console"] !== true) {
    const legalCommands = ["qqq"];
    consoleCommands.concat(scriptCommands).forEach((name) => {
      const command = findConsoleCommand(name);

      if (
        !command ||
        legalCommands.includes(command.longName.toLowerCase()) ||
        legalCommands.includes(command.shortName.toLowerCase())
      )
        return;
      command.execute = () => {
        printConsole(
          "You do not have permission to use this command ('" + name + "')"
        );
        return false;
      };
    });
  }
};
