import { settings, printConsole, findConsoleCommand } from "skyrimPlatform";
import { consoleCommands, scriptCommands } from "./consoleCommands";
import { MsgType } from "./messages";

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

enum CmdArgument {
  ObjectReference,
  BaseForm,
  Int,
  String,
}

type CmdName = "additem" | "placeatme";

const schemas = {
  additem: [CmdArgument.ObjectReference, CmdArgument.BaseForm, CmdArgument.Int],
  placeatme: [CmdArgument.ObjectReference, CmdArgument.BaseForm],
  disable: [CmdArgument.ObjectReference],
};

export const setUpConsoleCommands = (
  send: (msg: Record<string, unknown>) => void,
  localIdToRemoteId: (localId: number) => number
): void => {
  Object.keys(schemas).forEach((commandName: CmdName) => {
    const command = findConsoleCommand(commandName);
    if (!command)
      return printConsole(`Unable to find '${commandName}' command`);
    command.execute = (...args: number[] | string[]) => {
      const schema = schemas[commandName];
      if (args.length !== schema.length) {
        printConsole(
          `Mismatch found in the schema of '${commandName}' command`
        );
        return false;
      }
      for (let i = 0; i < args.length; ++i) {
        switch (schema[i]) {
          case CmdArgument.ObjectReference:
            args[i] = localIdToRemoteId(parseInt(`${args[i]}`));
            break;
        }
      }
      send({ t: MsgType.ConsoleCommand, data: { commandName, args } });
      return false;
    };
  });
};
