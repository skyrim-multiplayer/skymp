import { MsgType } from "../../messages";
import { ConnectionMessage } from "../events/connectionMessage";
import { FinishSpSnippetMessage } from "../messages/finishSpSnippetMessage";
import { SpSnippetMessage } from "../messages/spSnippetMessage";
import { ClientListener, CombinedController, Sp } from "./clientListener";

// TODO: refactor worldViewMisc into service
import { remoteIdToLocalId } from '../../view/worldViewMisc';
import { logError, logTrace } from "../../logging";

export class SpSnippetService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();
        this.controller.emitter.on("spSnippetMessage", (e) => this.onSpSnippetMessage(e));
        this.spAny = sp as Record<string, any>;
    }

    private onSpSnippetMessage(event: ConnectionMessage<SpSnippetMessage>): void {
        const msg = event.message;

        this.controller.once('update', async () => {
            this.run(msg)
                .then((res) => {
                    const isNoResultSnippet = msg.snippetIdx === 0xffffffff;
                    if (isNoResultSnippet) {
                        return;
                    }

                    if (res === undefined) {
                        res = null;
                    }

                    const message: FinishSpSnippetMessage = {
                        t: MsgType.FinishSpSnippet,
                        returnValue: res,
                        snippetIdx: msg.snippetIdx,
                    }

                    this.controller.emitter.emit("sendMessage", {
                        message: message,
                        reliability: "reliable"
                    });
                })
                .catch((e) => {
                    logError(this, 'SpSnippet ' + msg.class + ' ' + msg.function + ' failed ' + e);
                });
        });
    }

    private async run(snippet: SpSnippetMessage): Promise<any> {
        const functionLowerCase = snippet.function.toLowerCase();
        const classLowerCase = snippet.class.toLowerCase();

        // keep in sync with remoteServer.ts
        if (classLowerCase === "objectreference") {
            if (functionLowerCase === "setdisplayname") {
                let newName = snippet.arguments[0];
                if (typeof newName === "string") {

                    const selfId = remoteIdToLocalId(snippet.selfId);
                    const self = this.sp.ObjectReference.from(this.sp.Game.getFormEx(selfId));

                    const replaceValue = self?.getBaseObject()?.getName();

                    if (replaceValue !== undefined && replaceValue !== "%original_name%") {
                        // SP doesn't support String.replaceAll because Chakracore doesn't
                        while (newName.includes("%original_name%")) {
                            newName = newName.replace("%original_name%", replaceValue);
                        }
                        snippet.arguments[0] = newName;
                    }
                    else {
                        logError(this, "Couldn't get a replaceValue for SetDisplayName, snippet.selfId was", snippet.selfId.toString(16));
                    }
                }
                else {
                    logError(this, "Encountered SetDisplayName with non-string argument", newName);
                }
            }
        }

        if (classLowerCase === "skymphacks") {
            if (functionLowerCase === "additem" || functionLowerCase === "removeitem") {
                const form = this.sp.Form.from(this.deserializeArg(snippet.arguments[0]));
                if (form === null) {
                    logError(this, "Unable to find form with id " + snippet.arguments[0].formId.toString(16));
                    return;
                }

                const sign = snippet.function === "AddItem" ? "+" : "-";
                const count = snippet.arguments[1];

                let soundId = 0x334ab;
                if (form.getFormID() !== 0xf) {
                    soundId = 0x14115;
                }

                const sound = this.sp.Sound.from(this.sp.Game.getFormEx(soundId));
                if (sound !== null) {
                    const name = form.getName();
                    if (name.trim() === "") {
                        logTrace(this, "Sound will not be played because item has no name")
                    }
                    else {
                        sound.play(this.sp.Game.getPlayer());
                    }
                }
                else {
                    logError(this, "Unable to find sound with id " + soundId.toString(16));
                }

                if (count <= 0) {
                    logError(this, "Positive count expected, got " + count.toString());
                }
                else {
                    const name = form.getName();
                    if (name.trim() === "") {
                        logTrace(this, "Notification will not be shown because item has no name")
                    }
                    else {
                        this.sp.Debug.notification(sign + " " + name + " (" + count + ")");
                    }
                    logTrace(this, sign + " " + name + " (" + count + ")");
                }
            } else throw new Error("Unknown SkympHack - " + snippet.function);
            return;
        }
        return snippet.selfId ? this.runMethod(snippet) : this.runStatic(snippet);
    };

    private deserializeArg(arg: any) {
        if (typeof arg === "object") {
            const formId = remoteIdToLocalId(arg.formId);
            const form = this.sp.Game.getFormEx(formId);
            const gameObject = this.spAny[arg.type].from(form);
            return gameObject;
        }
        return arg;
    };

    private async runMethod(snippet: SpSnippetMessage): Promise<any> {
        const selfId = remoteIdToLocalId(snippet.selfId);
        const self = this.sp.Game.getFormEx(selfId);
        if (!self)
            throw new Error(
                `Unable to find form with id ${selfId.toString(16)}`
            );
        const selfCasted = this.spAny[snippet.class].from(self);
        if (!selfCasted)
            throw new Error(
                `Form ${selfId.toString(16)} is not instance of ${snippet.class}, form type is ${self.getType()}`
            );
        const f = selfCasted[snippet.function];
        return await f.apply(
            selfCasted,
            snippet.arguments.map((arg) => this.deserializeArg(arg))
        );
    };

    private async runStatic(snippet: SpSnippetMessage): Promise<any> {
        const papyrusClass = this.spAny[snippet.class];
        return await papyrusClass[snippet.function](
            ...snippet.arguments.map((arg) => this.deserializeArg(arg))
        );
    };

    private spAny: Record<string, any>;
};
