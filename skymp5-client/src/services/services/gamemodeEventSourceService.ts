import { localIdToRemoteId, remoteIdToLocalId } from "../../view/worldViewMisc";
import { ConnectionMessage } from "../events/connectionMessage";
import { CustomEventMessage } from "../messages/customEventMessage";
import { UpdateGamemodeDataMessage } from "../messages/updateGameModeDataMessage";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { MsgType } from "../../messages";

export class GamemodeEventSourceService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();

        if (Array.isArray(this.sp.storage['eventSourceContexts'])) {
            this.sp.storage['eventSourceContexts'] = this.sp.storage['eventSourceContexts'].filter(
                (ctx: Record<string, unknown>) => !ctx._expired,
            );
            (this.sp.storage['eventSourceContexts'] as any).forEach((ctx: any) => {
                this.setupEventSource(ctx);
            });
        }

        this.controller.emitter.on("updateGamemodeDataMessage", (e) => this.onUpdateGamemodeDataMessage(e));
    }

    private onUpdateGamemodeDataMessage(event: ConnectionMessage<UpdateGamemodeDataMessage>) {
        if (!Array.isArray(this.sp.storage['eventSourceContexts'])) {
            this.sp.storage['eventSourceContexts'] = [];
        } else {
            this.sp.storage['eventSourceContexts'].forEach((ctx: Record<string, unknown>) => {
                ctx.sendEvent = () => { };
                ctx._expired = true;
            });
        }
        const eventNames = Object.keys(event.message.eventSources);
        eventNames.forEach((eventName) => {
            try {
                const fn = new Function('ctx', event.message.eventSources[eventName]);
                const ctx = {
                    sp: this.sp,
                    sendEvent: (...args: unknown[]) => {
                        const message: CustomEventMessage = {
                            t: MsgType.CustomEvent,
                            args,
                            eventName
                        };
                        this.controller.emitter.emit("sendMessage", {
                            message: message,
                            reliability: "reliable"
                        });
                    },
                    getFormIdInServerFormat: (clientsideFormId: number) => {
                        return localIdToRemoteId(clientsideFormId);
                    },
                    getFormIdInClientFormat: (serversideFormId: number) => {
                        return remoteIdToLocalId(serversideFormId);
                    },
                    _fn: fn,
                    _eventName: eventName,
                    state: {},
                };
                (this.sp.storage['eventSourceContexts'] as Record<string, any>).push(ctx);
                this.setupEventSource(ctx);
            } catch (e) {
                this.sp.printConsole(`'eventSources.${eventName}' -`, e);
            }
        });
    }

    private setupEventSource = (ctx: any) => {
        this.controller.once('update', () => {
            try {
                ctx._fn(ctx);
                this.sp.printConsole(`'eventSources.${ctx._eventName}' - Added`);
            } catch (e) {
                this.sp.printConsole(`'eventSources.${ctx._eventName}' -`, e);
            }
        });
    };
};
