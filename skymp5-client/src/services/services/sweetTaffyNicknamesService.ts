import { NicknameCreateEvent } from "../events/nicknameCreateEvent";
import { NicknameDestroyEvent } from "../events/nicknameDestroyEvent";
import { ClientListener, CombinedController, Sp } from "./clientListener";

export class SweetTaffyNicknamesService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();

        controller.emitter.on("nicknameCreate", (e) => this.onNicknameCreate(e));
        controller.emitter.on("nicknameDestroy", (e) => this.onNicknameDestroy(e));
    }

    private onNicknameCreate(e: NicknameCreateEvent) {
        const storageNickname = typeof this.sp.storage["idTextNickname"] === 'object'
            ? this.sp.storage["idTextNickname"] as { [refrId: number]: number }
            : null;

        if (storageNickname === null && e.remoteRefrId) {
            this.sp.storage["idTextNickname"] = { [e.remoteRefrId]: e.textId };
        } else if (storageNickname !== null && e.remoteRefrId) {
            storageNickname[e.remoteRefrId] = e.textId;
            this.sp.storage["idTextNickname"] = storageNickname;
        }
    }

    private onNicknameDestroy(e: NicknameDestroyEvent) {
        const storageNickname = typeof this.sp.storage["idTextNickname"] === 'object'
            ? this.sp.storage["idTextNickname"] as { [refrId: number]: number }
            : null;

        if (storageNickname !== null && e.remoteRefrId) {
            delete storageNickname[e.remoteRefrId];
            this.sp.storage["idTextNickname"] = storageNickname;
        }
    }
};
