import { Inventory } from "../../sync/inventory";
import { ClientListener, CombinedController, Sp } from "./clientListener";

export class LastInvService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();
    }

    get lastInv() {
        return this._lastInv;
    }

    set lastInv(newValue: Inventory | undefined) {
        this._lastInv = newValue;
    }

    private _lastInv: Inventory | undefined;
}
