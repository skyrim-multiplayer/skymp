import { Inventory } from "skyrimPlatform";

export interface SetInventoryMessage {
    type: "setInventory";
    inventory: Inventory;
}
