import { Inventory } from "skyrimPlatform";
import { MsgType } from "../../messages";

export interface SetInventoryMessage {
    t: MsgType.SetInventory;
    inventory: Inventory;
}
