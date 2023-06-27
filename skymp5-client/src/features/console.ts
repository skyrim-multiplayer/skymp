import {
  findConsoleCommand,
  printConsole,
  settings,
  storage,
} from 'skyrimPlatform';

import { MsgType } from '../messages';

enum CmdArgument {
  ObjectReference,
  BaseForm,
  Int,
  String,
}

type CmdName = 'additem' | 'placeatme' | 'disable' | 'mp';

const schemas = {
  additem: [CmdArgument.ObjectReference, CmdArgument.BaseForm, CmdArgument.Int],
  placeatme: [CmdArgument.ObjectReference, CmdArgument.BaseForm],
  disable: [CmdArgument.ObjectReference],
  mp: [CmdArgument.ObjectReference, CmdArgument.String],
};

const immuneSchema = ['mp'];
const nonVanilaCommands = ['mp'];

const getCommandExecutor = (
  commandName: CmdName,
  send: (msg: Record<string, unknown>) => void,
  localIdToRemoteId: (localId: number) => number,
) => {
  return (...args: number[] | string[]) => {
    const schema = schemas[commandName];
    if (args.length !== schema.length && !immuneSchema.includes(commandName)) {
      printConsole(`Mismatch found in the schema of '${commandName}' command`);
      return false;
    }
    for (let i = 0; i < args.length; ++i) {
      switch (schema[i]) {
        case CmdArgument.ObjectReference:
          args[i] = localIdToRemoteId(parseInt(`${args[i]}`));
          break;
      }
    }
    printConsole('sent');
    send({ t: MsgType.ConsoleCommand, data: { commandName, args } });
    return false;
  };
};

export const setUpConsoleCommands = (
  send: (msg: Record<string, unknown>) => void,
  localIdToRemoteId: (localId: number) => number,
): void => {
  const command =
    findConsoleCommand(' ConfigureUM') || findConsoleCommand('test');
  if (command) {
    command.shortName = 'mp';
    command.execute = getCommandExecutor('mp', send, localIdToRemoteId) as (
      ...args: unknown[]
    ) => boolean;
  }

  (Object.keys(schemas) as any[]).forEach((commandName: CmdName) => {
    const command = findConsoleCommand(commandName);
    if (!command || nonVanilaCommands.includes(commandName)) return;
    command.execute = getCommandExecutor(
      commandName,
      send,
      localIdToRemoteId,
    ) as (...args: unknown[]) => boolean;
  });
};
