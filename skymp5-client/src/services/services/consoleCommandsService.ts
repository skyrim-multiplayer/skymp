import { MsgType } from "../../messages";

// TODO: refactor this out
import { localIdToRemoteId } from "../../view/worldViewMisc";

import { ClientListener, Sp, CombinedController } from "./clientListener";
import { SkympClient } from "./skympClient";

enum CmdArgument {
    ObjectReference,
    BaseForm,
    Int,
    String,
}

type CmdName = "additem" | "placeatme" | "disable" | "mp";

export class ConsoleCommandsService implements ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        this.schemas = ConsoleCommandsService.createSchemas();
        this.setupMpCommand();
        this.setupVanilaCommands();
    }

    private static createSchemas() {
        const schemas = new Map<CmdName, CmdArgument[]>();
        schemas.set("additem", [CmdArgument.ObjectReference, CmdArgument.BaseForm, CmdArgument.Int]);
        schemas.set("placeatme", [CmdArgument.ObjectReference, CmdArgument.BaseForm]);
        schemas.set("disable", [CmdArgument.ObjectReference]);
        schemas.set("mp", [CmdArgument.ObjectReference, CmdArgument.String]);
        return schemas;
    }

    private setupMpCommand() {
        const command = this.sp.findConsoleCommand(" ConfigureUM") || this.sp.findConsoleCommand("test");
        if (command === null) {
            return this.logError("command was null in setupMpCommand");
        }

        command.shortName = "mp";
        command.execute = this.getCommandExecutor("mp");
    }

    private setupVanilaCommands() {
        this.schemas.forEach((_, commandName) => {
            const command = this.sp.findConsoleCommand(commandName);
            if (command === null) {
                return this.logError(`command '${commandName}' was null in setupVanilaCommands`);
            }
            if (this.nonVanilaCommands.includes(commandName)) {
                return this.logTrace(`command '${commandName}' is non-vanila command`);
            }
            command.execute = this.getCommandExecutor(commandName);
        });
    }

    private getCommandExecutor(commandName: CmdName): (...args: unknown[]) => boolean {
        return (...args: unknown[]) => {
            // TODO: handle possible exceptions in this function
            const schema = this.schemas.get(commandName);
            if (schema === undefined) {
                this.logError(`Schema not found for command '${commandName}'`);
                return false;
            }

            if (args.length !== schema.length && !this.immuneSchema.includes(commandName)) {
                this.logError(`Mismatch found in the schema of '${commandName}' command`);
                return false;
            }
            for (let i = 0; i < args.length; ++i) {
                switch (schema[i]) {
                    case CmdArgument.ObjectReference:
                        args[i] = localIdToRemoteId(parseInt(`${args[i]}`));
                        break;
                }
            }

            const skympClient = this.controller.lookupListener("SkympClient") as SkympClient;
            const sendTarget = skympClient.getSendTarget();
            if (sendTarget === undefined) {
                this.logError("sendTarget was undefined in command executor");
                return false;
            }

            sendTarget.send({ t: MsgType.ConsoleCommand, data: { commandName, args } }, true);

            // Meant to be shown to user, not for logging
            this.sp.printConsole("sent");
            return false;
        };
    }

    // TODO: redirect this to spdlog
    private logError(error: string) {
        this.sp.printConsole("Error in ConsoleCommandsService:", error);
    }

    // TODO: redirect this to spdlog
    private logTrace(trace: string) {
        this.sp.printConsole("Trace in ConsoleCommandsService:", trace);
    }

    private readonly schemas: Map<CmdName, CmdArgument[]>;
    private readonly immuneSchema = ["mp"];
    private readonly nonVanilaCommands = ["mp"];
}
