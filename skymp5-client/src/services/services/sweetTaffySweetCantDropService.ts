import { ClientListener, CombinedController, Sp } from "./clientListener";

// TODO: move to the server/gamemode
export class SweetTaffySweetCantDropService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();
    }

    public canDropOrPutItem(itemId: number): boolean {
        const item = this.sp.Game.getFormEx(itemId);
        if (item !== null) {
            if (item.hasKeyword(this.sp.Keyword.getKeyword(this.cantDropKeyword))) {
                return false;
            }
        }
        return true;
    }

    private cantDropKeyword = "SweetCantDrop";
}
