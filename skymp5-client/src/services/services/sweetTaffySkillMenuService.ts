import { ActivateEvent, browser } from 'skyrimPlatform'
import { ClientListener, CombinedController, Sp } from './clientListener';

// TODO: move to server/gamemode
export class SweetTaffySkillMenuService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();

        if (!this.hasSweetPie()) {
            this.logTrace("SweetTaffy features disabled");
        }
        else {
            this.logTrace("SweetTaffy features enabled");
        }

        this.controller.once("activate", (e) => this.onActivate(e));
    }

    private onActivate(event: ActivateEvent) {
        if (!this.hasSweetPie()) return;

        if (!this.altars.includes(event.target.getBaseObject()?.getFormID() || -1)) return;

        browser.setFocused(true);

        const src = "window.dispatchEvent(new CustomEvent('initSkillMenu'))";
        browser.executeJavaScript(src);
    }

    private hasSweetPie(): boolean {
        const modCount = this.sp.Game.getModCount();
        for (let i = 0; i < modCount; ++i) {
            if (this.sp.Game.getModName(i).toLowerCase().includes('sweetpie')) {
                return true;
            }
        }
        return false;
    }

    private readonly altars =
        [
            0x00100780, 0x000D9883, 0x000D987B, 0x000D9881, 0x000D9887, 0x000FB997,
            0x000D9885, 0x000D987D, 0x00071854, 0x07187500, 0x0200c86b, 0x000d987f,
            0x07058d4e, 0x07058d53, 0x070A6912, 0x07058d51, 0x07058d50, 0x07058d4f,
            0x07058d4d, 0x07058d4c, 0x07058d4b, 0x0705e2fa, 0x07058d4a, 0x07058d49,
            0x0705e367, 0x07058d48, 0x07058d55, 0x07058d46, 0x07058d47, 0x0705e2b0,
            0x07058d45
        ];
}
