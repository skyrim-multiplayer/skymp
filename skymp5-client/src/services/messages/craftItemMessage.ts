import { Inventory } from "skyrimPlatform";
import { MsgType } from "../../messages";

export interface CraftItemMessage {
    t: MsgType.CraftItem,
    data: { 
        workbench: number, 
        craftInputObjects: Inventory, 
        resultObjectId: number
    }
}
