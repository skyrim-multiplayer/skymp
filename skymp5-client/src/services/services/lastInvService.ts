import { Inventory } from "../../sync/inventory";
import { ClientListener, CombinedController, Sp } from "./clientListener";

export class LastInvService implements ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
    }

    get lastInv() {
        return this._lastInv;
    }

    set lastInv(newValue: Inventory | undefined) {
        this._lastInv = newValue;
    }

    private _lastInv: Inventory | undefined;
}
