import { PlayerBowShotEvent } from "skyrimPlatform";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { MsgType } from "../../messages";

export class PlayerBowShotService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();
        this.controller.on("playerBowShot", (e) => this.onPlayerBowShot(e));
    }

    private onPlayerBowShot(e: PlayerBowShotEvent) {
        this.controller.emitter.emit("sendMessage", {
            message: {
                t: MsgType.PlayerBowShot,
                weaponId: e.weapon.getFormID(),
                ammoId: e.ammo.getFormID(),
                power: e.power,
                isSunGazing: e.isSunGazing
            },
            reliability: "unreliable"
        });
    }
};
