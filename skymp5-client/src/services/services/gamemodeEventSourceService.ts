import { localIdToRemoteId, remoteIdToLocalId } from "../../view/worldViewMisc";
import { ConnectionMessage } from "../events/connectionMessage";
import { CustomEventMessage } from "../messages/customEventMessage";
import { UpdateGamemodeDataMessage } from "../messages/updateGameModeDataMessage";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { MsgType } from "../../messages";
import { GamemodeApiEventSourceCtx } from "../messages_gamemode/gamemodeApiEventSourceCtx";

// The reason we use global skyrimPlatform is that this.sp may be limited, and gamemode api needs unlimited access to skyrimPlatform
// Sligthly different types
import * as skyrimPlatform from "skyrimPlatform";
import { logError, logTrace } from "../../logging";

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

        let eventNames = Object.keys(event.message.eventSources);

        let blockedEventSources = this.sp.settings["skymp5-client"]["blockedEventSources"];

        if (Array.isArray(blockedEventSources)) {
            blockedEventSources.forEach((blockedEventSource: unknown) => {
                eventNames = eventNames.filter((eventName) => eventName !== blockedEventSource);
                logTrace(this, `'eventSources`, blockedEventSource, `- Blocked by the client`);
            });
        }

        eventNames.forEach((eventName) => {
            try {
                const fn = new Function('ctx', event.message.eventSources[eventName]);
                const ctx: GamemodeApiEventSourceCtx = {
                    refr: undefined,
                    value: undefined,
                    _model: undefined,
                    _view: undefined,
                    i: -1,
                    get: (_propName: string) => {
                        throw new Error("ctx.get can't be used in event source");
                    },
                    respawn() {
                        throw new Error("ctx.respawn can't be used in event source");
                    },

                    sp: skyrimPlatform,
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
                logError(this, `'eventSources`, eventName, `-`, e);
            }
        });
    }

    private setupEventSource = (ctx: any) => {
        this.controller.once('update', () => {
            try {
                ctx._fn(ctx);
                logTrace(this, `'eventSources`, ctx._eventName, `- Added`);
            } catch (e) {
                logError(this, `'eventSources`, ctx._eventName, `-`, e);
            }
        });
    };
};
