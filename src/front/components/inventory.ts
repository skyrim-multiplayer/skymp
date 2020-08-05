import {
  getExtraContainerChanges,
  getContainer,
  BaseExtraList,
  ExtraHealth,
  ObjectReference,
  TESModPlatform,
  Game,
  printConsole,
  storage,
  ExtraCount,
  ExtraEnchantment,
  ExtraPoison,
  ExtraSoul,
  ExtraTextDisplayData,
  ExtraCharge,
  Enchantment,
  Potion,
  Actor,
  Weapon,
  Debug,
  Utility,
  Armor,
} from "skyrimPlatform";

export interface Extra {
  health?: number;
  enchantmentId?: number;
  maxCharge?: number;
  removeEnchantmentOnUnequip?: boolean;
  chargePercent?: number;
  name?: string;
  soul?: 0 | 1 | 2 | 3 | 4 | 5;
  poisonId?: number;
  poisonCount?: number;
  worn?: boolean;
  wornLeft?: boolean;
}

export interface BasicEntry {
  baseId: number;
  count: number;
}

export type Entry = BasicEntry & Extra;

// 'loxsword (Legendary)' => 'loxsword'
const getRealName = (s?: string): string => {
  if (!s) return s;

  const arr = s.split(" ");
  if (arr.length && arr[arr.length - 1].match(/^\(.*\)$/)) arr.pop();
  return arr.join(" ");
};

// 'aaaaaaaaaaaaaaaa' => 'aaa...'
const cropName = (s?: string): string => {
  if (!s) return s;

  const max = 128;
  return s.length >= max
    ? s
        .split("")
        .filter((x, i) => i < max)
        .join("")
        .concat("...")
    : s;
};

const extrasEqual = (a: Entry, b: Entry) => {
  return (
    a.health === b.health &&
    a.enchantmentId === b.enchantmentId &&
    a.maxCharge === b.maxCharge &&
    !!a.removeEnchantmentOnUnequip === !!b.removeEnchantmentOnUnequip &&
    a.chargePercent === b.chargePercent &&
    cropName(getRealName(a.name)) === cropName(getRealName(b.name)) &&
    a.soul === b.soul &&
    a.poisonId === b.poisonId &&
    a.poisonCount === b.poisonCount
  );
};

const hasExtras = (e: Entry): boolean => {
  return !extrasEqual(e, { baseId: 0, count: 0 });
};

export interface Inventory {
  entries: Entry[];
}

const extractExtraData = (
  extraList: BaseExtraList | null,
  out: Entry
): void => {
  (extraList || []).forEach((extra) => {
    switch (extra.type) {
      case "Health":
        out.health = Math.round((extra as ExtraHealth).health * 10) / 10;
        break;
      case "Count":
        out.count = (extra as ExtraCount).count;
        break;
      case "Enchantment":
        out.enchantmentId = (extra as ExtraEnchantment).enchantmentId;
        out.maxCharge = (extra as ExtraEnchantment).maxCharge;
        out.removeEnchantmentOnUnequip = (extra as ExtraEnchantment).removeOnUnequip;
        break;
      case "Charge":
        out.chargePercent = (extra as ExtraCharge).charge;
        break;
      case "Poison":
        out.poisonId = (extra as ExtraPoison).poisonId;
        out.poisonCount = (extra as ExtraPoison).count;
        break;
      case "Soul":
        out.soul = (extra as ExtraSoul).soul;
        break;
      case "TextDisplayData":
        out.name = (extra as ExtraTextDisplayData).name;
        break;
    }
  });
};

const getExtraContainerChangesAsInventory = (
  refr: ObjectReference
): Inventory => {
  const extraContainerChanges = getExtraContainerChanges(refr.getFormID());

  /** {
        baseId: 0x12e49,
        count: 1,
      },

      {
        baseId: 0x00012eb6,
        count: 1,
      },
      {
        baseId: 0x0001d4ec,
        count: 1,
      },
      {
        baseId: 0x000236a5, */
  printConsole({
    extraContainerChanges: extraContainerChanges.filter(
      (x) =>
        x.baseId == 0x12eb7 ||
        x.baseId == 0x12e49 ||
        x.baseId == 0x00012eb6 ||
        x.baseId == 0x0001d4ec ||
        x.baseId == 0x000236a5
    ),
  });

  const entries = new Array<Entry>();

  extraContainerChanges.forEach((changesEntry) => {
    const entry: Entry = {
      baseId: changesEntry.baseId,
      count: changesEntry.countDelta,
    };

    (changesEntry.extendDataList || []).forEach((extraList) => {
      const e: Entry = {
        baseId: entry.baseId,
        count: 1,
      };
      extractExtraData(extraList, e);
      entries.push(e);
      entry.count -= e.count;
    });

    if (entry.count !== 0) entries.push(entry);
  });

  return { entries };
};

const getBaseContainerAsInventory = (refr: ObjectReference): Inventory => {
  return { entries: getContainer(refr.getBaseObject().getFormID()) };
};

const sumInventories = (lhs: Inventory, rhs: Inventory): Inventory => {
  const leftEntriesWithExtras = lhs.entries.filter((e) => hasExtras(e));
  const rightEntriesWithExtras = rhs.entries.filter((e) => hasExtras(e));
  const leftEntriesSimple = lhs.entries.filter((e) => !hasExtras(e));
  const rightEntriesSimple = rhs.entries.filter((e) => !hasExtras(e));

  leftEntriesSimple.forEach((e) => {
    const matching = rightEntriesSimple.find((x) => x.baseId === e.baseId);
    if (matching) {
      e.count += matching.count;
      matching.count = 0;
    }
  });

  return {
    entries: leftEntriesWithExtras
      .concat(rightEntriesWithExtras)
      .concat(leftEntriesSimple)
      .concat(rightEntriesSimple)
      .filter((e) => e.count !== 0),
  };
};

const getDiff = (lhs: Inventory, rhs: Inventory): Inventory => {
  const lhsCopy: Inventory = JSON.parse(JSON.stringify(lhs));
  const rhsCopy: Inventory = JSON.parse(JSON.stringify(rhs));

  rhsCopy.entries.forEach((e) => {
    const sameFromLeft = lhsCopy.entries.find(
      (x) => x.baseId === e.baseId && extrasEqual(x, e)
    );
    if (sameFromLeft) {
      sameFromLeft.count -= e.count;
    } else {
      lhsCopy.entries.push(e);
      lhsCopy.entries[lhsCopy.entries.length - 1].count *= -1;
    }
  });

  return { entries: lhsCopy.entries.filter((x) => x.count !== 0) };
};

const squash = (inv: Inventory): Inventory => {
  const res = new Array<Entry>();
  inv.entries.forEach((e) => {
    const same = res.find((x) => e.baseId === x.baseId && extrasEqual(x, e));
    if (same) {
      same.count += e.count;
    } else {
      res.push(JSON.parse(JSON.stringify(e)));
    }
  });
  return { entries: res.filter((x) => x.count !== 0) };
};

export const getInventory = (refr: ObjectReference): Inventory => {
  return squash(
    sumInventories(
      getBaseContainerAsInventory(refr),
      getExtraContainerChangesAsInventory(refr)
    )
  );
};

const basesReset = (): Set<number> => {
  if (storage["basesResetExists"] !== true) {
    storage["basesResetExists"] = true;
    storage["basesReset"] = new Set<number>();
  }
  return storage["basesReset"];
};

const resetBase = (refr: ObjectReference): void => {
  const base = refr.getBaseObject();
  const baseId = base ? base.getFormID() : 0;
  if (!basesReset().has(baseId)) {
    basesReset().add(baseId);
    TESModPlatform.resetContainer(base);

    refr.removeAllItems(null, false, true);
  }
};

export const applyInventory = (
  refr: ObjectReference,
  newInventory: Inventory
): void => {
  resetBase(refr);
  const diff = getDiff(newInventory, getInventory(refr)).entries;
  if (diff.length) printConsole("diff " + diff.length);
  diff.sort((a, b) => (a.count < b.count ? -1 : 1));
  diff.forEach((e) => {
    printConsole("addItemEx", e.baseId.toString(16), e);

    const absCount = Math.abs(e.count);

    let queueNiNodeUpdateNeeded = false;
    //let noQueuePlz = false;

    printConsole(e);
    for (let i = 0; i < absCount; ++i) {
      if (e.worn || e.wornLeft) {
        // refr.setAnimationVariableInt("IsEquipping", 1);

        TESModPlatform.pushWornState(!!e.worn, !!e.wornLeft);
        queueNiNodeUpdateNeeded = true;
        /*if (e.worn && Armor.from(Game.getFormEx(e.baseId))) {
          queueNiNodeUpdateNeeded = true;
        }
        if ((e.wornLeft || e.worn) && !Armor.from(Game.getFormEx(e.baseId))) {
          noQueuePlz = true;
        }*/
      }
      TESModPlatform.addItemEx(
        refr,
        Game.getFormEx(e.baseId),
        e.count / absCount,
        e.health ? e.health : 1,
        e.enchantmentId
          ? Enchantment.from(Game.getFormEx(e.enchantmentId))
          : null,
        e.maxCharge ? e.maxCharge : 0,
        !!e.removeEnchantmentOnUnequip,
        e.chargePercent ? e.chargePercent : 0,
        e.name ? cropName(e.name) : "",
        e.soul ? e.soul : 0,
        e.poisonId ? Potion.from(Game.getFormEx(e.poisonId)) : null,
        e.poisonCount ? e.poisonCount : 0
      );

      /*if (e.worn && Weapon.from(Game.getFormEx(e.baseId))) {
        const refrId = refr.getFormID();
        Utility.wait(0).then(() => {
          const r = Actor.from(Game.getFormEx(refrId));
          if (r) {
            printConsole("ddd");
            TESModPlatform.updateEquipment(r, Game.getFormEx(e.baseId), true);
          }
        });
      }

      if (e.wornLeft && Weapon.from(Game.getFormEx(e.baseId))) {
        const refrId = refr.getFormID();
        Utility.wait(0).then(() => {
          const r = Actor.from(Game.getFormEx(refrId));
          if (r) {
            TESModPlatform.updateEquipment(r, Game.getFormEx(e.baseId), false);
          }
        });
      }*/
    }

    if (queueNiNodeUpdateNeeded) {
      const ac = Actor.from(refr);
      if (ac) {
        ac.queueNiNodeUpdate();

        /*const id = ac.getFormID();
        Utility.wait(0.0).then(() => {
          const r = Actor.from(Game.getFormEx(id));
          if (r) {
            //r.queueNiNodeUpdate();
          }
        });
        Utility.wait(0.3).then(() => {
          const r = Actor.from(Game.getFormEx(id));
          if (r) {
            //TESModPlatform.updateEquipment(r, Game.getFormEx(e.baseId), false);
            r.queueNiNodeUpdate();
          }
        });*/

        //ac.queueNiNodeUpdate();
      }
    }
  });
};
