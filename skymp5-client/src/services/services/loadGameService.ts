import { ClientListener, CombinedController, Sp } from "./clientListener";
import { ChangeFormNpc } from "skyrimPlatform";

export class LoadGameService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();
        this.controller.on("loadGame", () => this.onLoadGame());
    }

    public loadGame(pos: number[], rot: number[], worldOrCell: number, changeFormNpc?: ChangeFormNpc, loadOrder?: string[], time?: { seconds: number, minutes: number, hours: number }) {

        try {
            // @ts-ignore
            this.sp.loadGame(pos, rot, worldOrCell, changeFormNpc, loadOrder, time);
        }
        catch (e) {
            // Hotfix non-vanilla headparts bug
            // @ts-ignore
            this.sp.loadGame(pos, rot, worldOrCell, undefined, loadOrder, time);
        }
        this._isCausedBySkyrimPlatform = true;
    }

    private onLoadGame() {
        try {
            const gameLoadEvent = {
                isCausedBySkyrimPlatform: this._isCausedBySkyrimPlatform
            };
            this.controller.emitter.emit("gameLoad", gameLoadEvent);
        } catch (e) {
            this.controller.once("tick", () => {
                this._isCausedBySkyrimPlatform = false;
            });
            throw e;
        }
        this.controller.once("tick", () => {
            this._isCausedBySkyrimPlatform = false;
        });
    }

    private _isCausedBySkyrimPlatform = false;
}
