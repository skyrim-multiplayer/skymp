import { FormModel } from "../../view/model";
import { ConnectionMessage } from "../events/connectionMessage";
import { CreateActorMessage } from "../messages/createActorMessage";
import { UpdateGamemodeDataMessage } from "../messages/updateGameModeDataMessage";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { RemoteServer } from "./remoteServer";
import { ObjectReference } from "skyrimPlatform";
import { FormViewArray } from "../../view/formViewArray";

// TODO: refactor
import { localIdToRemoteId, remoteIdToLocalId } from "../../view/worldViewMisc";

// TODO: use this.sp instead
import * as skyrimPlatform from "skyrimPlatform";

export interface GamemodeApiCtx {
    refr: ObjectReference | undefined;
    value: unknown;
    _model: FormModel | undefined;
    sp: typeof skyrimPlatform,
    state: Record<string, unknown> | undefined;
    _view: FormViewArray | undefined;
    i: number;
    getFormIdInServerFormat: (clientsideFormId: number) => number;
    getFormIdInClientFormat: (serversideFormId: number) => number;
    get: (propName: string) => unknown;
    respawn: () => void;
}

export class GamemodeUpdateService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();

        this.controller.emitter.on("updateGamemodeDataMessage", (e) => this.onUpdateGamemodeDataMessage(e));
        this.controller.emitter.on("createActorMessage", (e) => this.onCreateActorMessage(e));

        this.controller.on("tick", () => this.onTick());
        this.controller.on("update", () => this.onUpdate());

        this.updateOwnerCtx = {
            sp: skyrimPlatform,
            refr: undefined,
            value: undefined,
            _model: undefined,
            getFormIdInServerFormat: (clientsideFormId: number) => {
                return localIdToRemoteId(clientsideFormId);
            },
            getFormIdInClientFormat: (serversideFormId: number) => {
                return remoteIdToLocalId(serversideFormId);
            },
            get(propName: string) {
                return (this._model as Record<string, any>)[propName];
            },
            state: {},
            _view: undefined,
            i: -1,
            respawn: () => { throw new Error("ctx.respawn is not available for updateOwner"); }
        };

        this.updateNeighborCtx = {
            refr: undefined,
            value: undefined,
            _model: undefined,
            sp: skyrimPlatform,
            state: undefined,
            _view: undefined,
            i: -1,
            getFormIdInServerFormat: (clientsideFormId: number) => {
                return localIdToRemoteId(clientsideFormId);
            },
            getFormIdInClientFormat: (serversideFormId: number) => {
                return remoteIdToLocalId(serversideFormId);
            },
            get(propName: string) {
                return (this._model as Record<string, any>)[propName];
            },
            respawn() {
                this._view!.destroyForm(this.i);
            }
        };

        this.updateNeighborFunctionsKeys = [];
        this.updateNeighborFunctions = {};
    }

    setFormViewArray(formViewArray: FormViewArray) {
        this.updateNeighborCtx._view = formViewArray;
    }

    setI(i: number) {
        this.updateNeighborCtx.i = i;
    }

    updateNeighbor(refr: ObjectReference, model: FormModel, state: Record<string, unknown>) {
        for (const key of this.updateNeighborFunctionsKeys) {
            const v = (model as Record<string, unknown>)[key];
            // According to docs:
            // In `updateOwner`/`updateNeighbor` equals to a value of a currently processed property.
            // Can't be `undefined` here, since updates are not received for `undefined` property values.
            // In other contexts is always `undefined`.
            if (v !== undefined) {
                this.updateNeighborCtx.refr = refr;
                this.updateNeighborCtx.value = v;
                this.updateNeighborCtx._model = model;
                this.updateNeighborCtx.state = state;
                const f = this.updateNeighborFunctions[key];
                // Actually, 'f' should always be a valid function, but who knows
                try {
                    if (f) f(this.updateNeighborCtx);
                } catch (e) {
                    this.sp.printConsole(`'updateNeighbor.${key}' - `, e);
                }
            }
        }
    }

    private onUpdateGamemodeDataMessage(event: ConnectionMessage<UpdateGamemodeDataMessage>) {
        this.sp.storage['updateNeighborFunctions'] = undefined;
        this.sp.storage['updateOwnerFunctions'] = undefined;

        this.updateGamemodeUpdateFunctions(
            'updateNeighborFunctions',
            event.message.updateNeighborFunctions || {},
        );
        this.updateGamemodeUpdateFunctions(
            'updateOwnerFunctions',
            event.message.updateOwnerFunctions || {},
        );
    }

    private onCreateActorMessage(event: ConnectionMessage<CreateActorMessage>) {
        if (!event.message.isMe) {
            return;
        }

        // Otherwise worldModel.playerCharacterFormIdx may be unassigned
        this.controller.once("tick", () => {

            const remoteServer = this.controller.lookupListener(RemoteServer);

            const worldModel = remoteServer.getWorldModel();

            const myFormModel: FormModel | undefined = worldModel.forms[worldModel.playerCharacterFormIdx];

            if (!myFormModel) {
                return this.logError(`Unable to find formModel with index ${worldModel.playerCharacterFormIdx}`);
            }

            this.setOwnerModel(myFormModel);
        });
    }

    private onTick() {
        const keys = this.sp.storage["updateNeighborFunctions_keys"] as Array<string>;
        if (keys && Array.isArray(keys)) {
            this.updateNeighborFunctionsKeys = keys;
        } else {
            this.updateNeighborFunctionsKeys = [];
        }

        // TODO: Shouldn't we check storage value before assignment?
        this.updateNeighborFunctions = this.sp.storage["updateNeighborFunctions"] as Record<
            string,
            any
        >;
    }

    private onUpdate() {
        const playerRef = this.sp.ObjectReference.from(this.sp.Game.getPlayer()) as ObjectReference;

        let keys = this.sp.storage["updateOwnerFunctions_keys"] as Array<string>;
        if (!keys || !Array.isArray(keys)) {
            keys = [];
        }
        const funcs = this.sp.storage["updateOwnerFunctions"];

        if (this.sp.storage["ownerModelSet"] !== true) return;

        const ownerModel = this.sp.storage["ownerModel"] as FormModel;

        for (const propName of keys) {
            const f = (funcs as Record<string, any>)[propName];
            // Actually, must always be a valid funciton, but who knows
            if (!f) continue;

            this.updateOwnerCtx._model = ownerModel;
            if (!this.updateOwnerCtx._model) continue;

            this.updateOwnerCtx.value = (this.updateOwnerCtx._model as Record<string, unknown>)[propName];
            if (this.updateOwnerCtx.value === undefined) continue;

            this.updateOwnerCtx.refr = playerRef;
            this.updateOwnerCtx._model = ownerModel;
            try {
                if (f) f(this.updateOwnerCtx);
            } catch (e) {
                this.sp.printConsole(`'updateOwner.${propName}' - `, e);
            }
        }
    }

    private setOwnerModel(ownerModel: FormModel) {
        this.sp.storage["ownerModel"] = ownerModel;
        this.sp.storage["ownerModelSet"] = true;
    };

    private updateGamemodeUpdateFunctions(
        storageVar: string,
        functionSources: Record<string, string>,
    ) {
        this.sp.storage[storageVar] = JSON.parse(JSON.stringify(functionSources));
        for (const propName of Object.keys(functionSources)) {
            try {
                (this.sp.storage[storageVar] as any)[propName] = new Function(
                    'ctx',
                    (this.sp.storage[storageVar] as any)[propName],
                );
                const emptyFunction = functionSources[propName] === '';
                if (emptyFunction) {
                    delete (this.sp.storage[storageVar] as any)[propName];
                    this.sp.printConsole(`'${storageVar}.${propName}' -`, 'Added empty');
                } else {
                    this.sp.printConsole(`'${storageVar}.${propName}' -`, 'Added');
                }
            } catch (e) {
                this.sp.printConsole(`'${storageVar}.${propName}' -`, e);
            }
        }
        this.sp.storage[`${storageVar}_keys`] = Object.keys(this.sp.storage[storageVar] as any);
    }

    private updateOwnerCtx: GamemodeApiCtx;

    private updateNeighborCtx: GamemodeApiCtx;
    private updateNeighborFunctionsKeys: string[];
    private updateNeighborFunctions: Record<string, any>;
};
