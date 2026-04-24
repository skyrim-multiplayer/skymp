import { logError, logTrace } from "../../logging";
import { ConnectionMessage } from "../events/connectionMessage";
import { CustomPacketMessage } from "../messages/customPacketMessage";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { NetworkingService } from "./networkingService";
import { ServerJsVerificationService } from "./serverJsVerificationService";
import { once, printConsole } from "skyrimPlatform";

export class SweetTaffyEvalService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();


        this.controller.emitter.on("customPacketMessage", (e) => this.onCustomPacketMessage(e));
    }

    private onCustomPacketMessage(event: ConnectionMessage<CustomPacketMessage>): void {
        const msg = event.message;

        let msgContent: Record<string, unknown> = {};

        try {
            msgContent = JSON.parse(msg.contentJsonDump);
        } catch (e) {
            if (e instanceof SyntaxError) {
                logError(this, "onCustomPacketMessage failed to parse JSON", e.message, "json:", msg.contentJsonDump);
                return;
            } else {
                throw e;
            }
        }

        switch (msgContent["customPacketType"]) {
            case 'eval':
                const code = msgContent["code"];

                if (typeof code !== "string") {
                    logError(this, "onCustomPacketMessage expected 'code' to be a string", "code:", code);
                    break;
                }
                this.processEval(code);
                break;
        }
    }

    private processEval(code: string): void {
        // Consider removing try catch
        try {
            const src = code;

            const serverJsVerificationService = this.controller.lookupListener(ServerJsVerificationService);

            const prefix = "// skymp:sig:y:GM";

            const result = serverJsVerificationService.verifyServerJs(src, prefix);

            if (result.src && !result.error) {
                once("update", () => {
                    eval("var ctx = { sp: skyrimPlatform };\n" + result.src);
                });
                printConsole("Eval executed successfully");
            } else {
                logError(this, "processEval failed server JS verification", "error:", result.error, "src:", src);
            }

        } catch (e) {
            logError(this, "processEval error", e);
        }
    }
}
