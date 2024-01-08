import { BasicEntry } from "../../sync/inventory";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { ContainerChangedEvent, Form } from "skyrimPlatform";
import { CreateActorMessage } from "../messages/createActorMessage";
import { ConnectionMessage } from "../events/connectionMessage";
import { SetInventoryMessage } from "../messages/setInventoryMessage";

export class SweetTaffyDynamicPerksService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();
    controller.on('containerChanged', (e) => this.onContainerChanged(e));
    controller.emitter.on("setInventoryMessage", (e) => this.onSetInventoryMessage(e));
    controller.emitter.on("createActorMessage", (e) => this.onCreateActorMessage(e));
  }

  private onContainerChanged(e: ContainerChangedEvent) {
    if (!this.hasSweetPie()) return;

    if (e.oldContainer && e.newContainer) {
      if (e.newContainer.getFormID() === 0x14 && e.numItems > 0) {
        this.handlePlayerInventoryChanged({
          baseId: e.baseObj.getFormID(),
          count: e.numItems,
        });
      }
    }
  }

  private onSetInventoryMessage(e: ConnectionMessage<SetInventoryMessage>) {
    if (!this.hasSweetPie()) return;

    const entries = e.message.inventory.entries;

    if (entries.length === 0) {
      return this.logTrace("Received SetInventoryMessage with empty inventory")
    }

    this.controller.once("update", () => {
      entries.forEach(entry => this.handlePlayerInventoryChanged(entry));
    });
  }

  private onCreateActorMessage(e: ConnectionMessage<CreateActorMessage>) {
    if (!this.hasSweetPie()) return;

    if (!e.message.isMe) return;

    const entries = e.message.inventory?.entries;
    if (entries === undefined) {
      return this.logTrace("Received CreateActorMessage without inventory");
    }

    if (entries.length === 0) {
      return this.logTrace("Received CreateActorMessage with empty inventory")
    }

    this.controller.once("update", () => {
      entries.forEach(entry => this.handlePlayerInventoryChanged(entry));
    });
  }

  private handlePlayerInventoryChanged(entry: BasicEntry): void {
    if (!this.hasSweetPie()) return;

    const item = this.sp.Game.getFormEx(entry.baseId);
    if (!item) return;
    if (entry.count <= 0) return;

    const perkIds = this.getPerkIdsByKeyword(item);
    if (perkIds) {
      const playerId = 0x14;
      this.requestAddPerksToActors([playerId], perkIds);
    }
  }

  private requestAddPerksToActors = (actorIds: number[], perkIds: number[]): void => {
    this.controller.once("update", () => {
      const actors = actorIds.map(actorId => this.sp.Actor.from(this.sp.Game.getFormEx(actorId))).filter(actor => actor !== null);
      const perks = perkIds.map(perkId => this.sp.Perk.from(this.sp.Game.getFormEx(perkId))).filter(perk => perk !== null);

      actors.forEach(actor => perks.forEach(perk => actor?.addPerk(perk)));
    });
  }

  private getPerkIdsByKeyword(item: Form): number[] | null {
    const result = new Array<number>();
    this.getKeywordPerkMap().forEach((v, k) => {
      const keyWord = this.sp.Keyword.getKeyword(k);
      if (item.hasKeyword(keyWord)) {
        result.push(v);
      }
    });

    return result.length > 0 ? result : null;
  }

  // TODO: move this to config
  private getKeywordPerkMap() {
    return new Map<string, number>([
      // Стрелец
      ["SweetPerkBow1", 0x1036F0],
      ["SweetPerkBow2", 0x58F61],
      ["SweetPerkBow3", 0x105F19],
      ["SweetPerkBow4", 0x58F63],
      // Убийца
      ["SweetPerkSneak1", 0x58210],
      ["SweetPerkSneak2", 0x105F23],
      ["SweetPerkSneak3", 0x58208],
      ["SweetPerkSneak4", 0x58211],
      // Пехотинец
      ["SweetPerkArmorLight1", 0x105F22],
      ["SweetPerkArmorLight2", 0x51B1C],
      // Зачарователь
      ["SweetPerkEnchant2", 0x108A44],
      ["SweetPerkEnchant1", 0x58F7C],
      // Латник
      ["SweetPerkArmorHeavy1", 0xBCD2B],
      ["SweetPerkArmorHeavy2", 0x58F6D],
      // Щитоносец
      ["SweetPerkBlock1", 0x58F67],
      ["SweetPerkBlock2", 0x106253],
      ["SweetPerkBlock3", 0x58F6A],
      ["SweetPerkBlock4", 0x58F69],
      // Мечник, клинок, рубака, крушитель, монах, копейщик, стражник (двуручное оружие)
      ["SweetPerk1Hand2Hand1", 0x58F6F],
      ["SweetPerk2Hand2", 0xCB407],
      ["SweetPerk2Hand3", 0x96590],
      ["SweetPerk2Hand4", 0x52D51],
      // Ассасин, разведчик, шпион, предшественник, рыцарь, разбойник, берсерк, громила (одноручное оружие)
      ["SweetPerk1Hand2Hand1", 0x58F6F],
      ["SweetPerk1Hand2", 0xCB406],
      ["SweetPerk1Hand3", 0x106256],
      ["SweetPerk1Hand4", 0x52D50],
    ]);
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
}
