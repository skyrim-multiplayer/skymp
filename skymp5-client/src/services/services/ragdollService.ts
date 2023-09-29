import { Actor } from "skyrimPlatform";
import { ClientListener, CombinedController, Sp } from "./clientListener";

export class RagdollService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();
        this.controller.once("update", () => this.onceUpdate());
    }

    // TODO: think about tracking ragdoll state of player
    public safeRemoveRagdollFromWorld = (
        actor: Actor,
        afterRemoveCallback: () => void
    ) => {
        this.setLocalDamageMult(0);
        actor.forceRemoveRagdollFromWorld().then(() => {
            this.controller.once("update", () => {
                this.setLocalDamageMult(this.defaultLocalDamageMult);
                afterRemoveCallback();
            });
        });
    };

    private onceUpdate() {
        this.setLocalDamageMult(this.defaultLocalDamageMult);
    }

    private setLocalDamageMult(damageMult: number) {
        this.sp.Game.setGameSettingFloat("fDiffMultHPToPCE", damageMult);
        this.sp.Game.setGameSettingFloat("fDiffMultHPToPCH", damageMult);
        this.sp.Game.setGameSettingFloat("fDiffMultHPToPCL", damageMult);
        this.sp.Game.setGameSettingFloat("fDiffMultHPToPCN", damageMult);
        this.sp.Game.setGameSettingFloat("fDiffMultHPToPCVE", damageMult);
        this.sp.Game.setGameSettingFloat("fDiffMultHPToPCVH", damageMult);
    }

    private readonly defaultLocalDamageMult = 1;
}
