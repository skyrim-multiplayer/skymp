import { ClientListener, Sp, CombinedController } from "./clientListener";

export class SweetTaffyStaticPerksService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();
        this.controller.once("update", () => this.onceUpdate());
    }

    private onceUpdate() {
        this.addStaticPerksToThePlayer();
    }

    private addStaticPerksToThePlayer() {
        const player = this.sp.Game.getPlayer()!;

        const allStaticPerkIds = this.stealth.concat(this.magic).concat(this.warrior);

        allStaticPerkIds.forEach(perkId => {
            player.addPerk(this.sp.Perk.from(this.sp.Game.getFormEx(perkId)));
        });
    }

    // TODO: move this to config
    private readonly stealth = [0xBE126, 0xC07C6, 0xC07C7, 0xC07C8, 0xC07C9];
    private readonly magic = [0x58213, 0x105F24, 0x58214];
    private readonly warrior = [0xCB40D, 0xCB40F, 0xCB414, 0xCB411, 0xCB412, 0xCB410, 0xCB40E];
}
