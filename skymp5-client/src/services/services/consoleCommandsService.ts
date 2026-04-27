import { logError, logTrace } from "../../logging";
import { MsgType } from "../../messages";

// TODO: refactor this out
import { localIdToRemoteId } from "../../view/worldViewMisc";

import { ClientListener, Sp, CombinedController } from "./clientListener";

enum CmdArgument {
    ObjectReference,
    BaseForm,
    Int,
    String,
}

type CmdName = "additem" | "equipitem" | "placeatme" | "disable" | "mp";

export class ConsoleCommandsService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();
        this.schemas = ConsoleCommandsService.createSchemas();
        this.setupMpCommand();
        this.setupVanilaCommands();
    }

    private static createSchemas() {
        const schemas = new Map<CmdName, CmdArgument[]>();
        schemas.set("additem", [CmdArgument.ObjectReference, CmdArgument.BaseForm, CmdArgument.Int]);
        schemas.set("equipitem", [CmdArgument.ObjectReference, CmdArgument.BaseForm]);
        schemas.set("placeatme", [CmdArgument.ObjectReference, CmdArgument.BaseForm]);
        schemas.set("disable", [CmdArgument.ObjectReference]);
        schemas.set("mp", [CmdArgument.ObjectReference, CmdArgument.String]);
        return schemas;
    }

    private setupMpCommand() {
        const command = this.sp.findConsoleCommand(" ConfigureUM") || this.sp.findConsoleCommand("test");
        if (command === null) {
            logError(this, "command was null in setupMpCommand");
            return;
        }

        command.shortName = "mp";
        command.execute = this.getCommandExecutor("mp");
    }

    private setupVanilaCommands() {
        logTrace(this, `Setting up vanila commands`);
        this.schemas.forEach((_, commandName) => {
            logTrace(this, `Setting up command`, commandName);
            const command = this.sp.findConsoleCommand(commandName);
            if (command === null) {
                logError(this, `command`, commandName, `was null in setupVanilaCommands`);
                return;
            }
            if (this.nonVanilaCommands.includes(commandName)) {
                logTrace(this, `command`, commandName, ` is non-vanila command`);
                return;
            }
            command.execute = this.getCommandExecutor(commandName);
        });
        logTrace(this, `Vanila commands set up`);
    }

    private getCommandExecutor(commandName: CmdName): (...args: unknown[]) => boolean {
        return (...args: unknown[]) => {
            // TODO: handle possible exceptions in this function
            const schema = this.schemas.get(commandName);
            if (schema === undefined) {
                logError(this, `Schema not found for command`, commandName);
                return false;
            }

            const normalizedArgs = commandName === "mp"
                ? this.normalizeMpArgs(args)
                : [...args];

            if (normalizedArgs.length !== schema.length && !this.immuneSchema.includes(commandName)) {
                logError(this, `Mismatch found in the schema of`, commandName, `command`);
                return false;
            }
            for (let i = 0; i < normalizedArgs.length; ++i) {
                switch (schema[i]) {
                    case CmdArgument.ObjectReference:
                        normalizedArgs[i] = localIdToRemoteId(parseInt(`${normalizedArgs[i]}`));
                        break;
                }
            }

            for (let i = 0; i < normalizedArgs.length; ++i) {
                if (typeof normalizedArgs[i] !== "string" && typeof normalizedArgs[i] !== "number") {
                    logError(this, `Bad argument type in command`, commandName, `argument index`, i);
                    return false;
                }
            }

            this.controller.emitter.emit("sendMessage", {
                message: {
                    t: MsgType.ConsoleCommand,
                    data: {
                        commandName,
                        args: normalizedArgs as (string | number)[]
                    }
                },
                reliability: "reliable"
            });

            // Meant to be shown to user, not for logging
            this.sp.printConsole("sent");
            return false;
        };
    }

    private normalizeMpArgs(args: unknown[]): unknown[] {
        if (args.length === 0) {
            return [0x14];
        }

        const [firstArg] = args;
        const normalizedFirstArg = `${firstArg}`.trim();
        if (normalizedFirstArg !== "" && !Number.isNaN(parseInt(normalizedFirstArg, 10))) {
            return [...args];
        }

        return [0x14, ...args];
    }

    private readonly schemas: Map<CmdName, CmdArgument[]>;
    private readonly immuneSchema = ["mp"];
    private readonly nonVanilaCommands = ["mp"];
}
