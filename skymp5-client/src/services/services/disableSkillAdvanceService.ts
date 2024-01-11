import { ActorValue } from "skyrimPlatform";
import { ClientListener, Sp, CombinedController } from "./clientListener";

export class DisableSkillAdvanceService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();
        this.controller.once("update", () => this.onceUpdate());
    }

    private onceUpdate() {
        // turn off player level exp
        this.sp.Game.setGameSettingFloat("fXPPerSkillRank", 0);

        // turn off skill exp
        this.turnOffSkillLocalExp(ActorValue.Alteration);
        this.turnOffSkillLocalExp(ActorValue.Conjuration);
        this.turnOffSkillLocalExp(ActorValue.Destruction);
        this.turnOffSkillLocalExp(ActorValue.Illusion);
        this.turnOffSkillLocalExp(ActorValue.Restoration);
        this.turnOffSkillLocalExp(ActorValue.Enchanting);
        this.turnOffSkillLocalExp(ActorValue.OneHanded);
        this.turnOffSkillLocalExp(ActorValue.TwoHanded);
        this.turnOffSkillLocalExp(ActorValue.Archery);
        this.turnOffSkillLocalExp(ActorValue.Block);
        this.turnOffSkillLocalExp(ActorValue.Smithing);
        this.turnOffSkillLocalExp(ActorValue.HeavyArmor);
        this.turnOffSkillLocalExp(ActorValue.LightArmor);
        this.turnOffSkillLocalExp(ActorValue.Pickpocket);
        this.turnOffSkillLocalExp(ActorValue.Lockpicking);
        this.turnOffSkillLocalExp(ActorValue.Sneak);
        this.turnOffSkillLocalExp(ActorValue.Alchemy);
        this.turnOffSkillLocalExp(ActorValue.Speech);
    }

    private turnOffSkillLocalExp(av: ActorValue): void {
        const avi = this.sp.ActorValueInfo.getActorValueInfoByID(av);
        if (avi === null) {
            this.logError(`Not found ActorValueInfo for actor value ${av}`);
            return;
        }
        avi.setSkillUseMult(0);
        avi.setSkillOffsetMult(0);
    };
}
